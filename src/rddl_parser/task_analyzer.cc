#include "task_analyzer.h"

#include "csp.h"
#include "evaluatables.h"
#include "rddl.h"

#include "utils/system_utils.h"
#include "utils/timer.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

using namespace std;

TaskAnalyzer::TaskAnalyzer(RDDLTask* _task) : task(_task) {}

void TaskAnalyzer::analyzeTask(int numStates, int numSimulations, double timeout, bool output) {
    Timer t;
    // Determine task properties
    if (output) {
        cout << "    Determining task properties..." << endl;
    }
    determineTaskProperties();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    // Approximate or calculate the min and max reward
    if (output) {
        cout << "    Calculating min and max reward..." << endl;
    }
    calculateMinAndMaxReward();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    if (output) {
        cout << "    Performing random walks..." << endl;
    }
    State currentState(task->CPFs);
    int remainingSteps = task->horizon;

    for (int simCounter = 0; simCounter < numSimulations;) {
        State nextState(task->CPFs.size());
        double reward = 0.0;
        analyzeStateAndApplyAction(currentState, nextState, reward);

        encounteredStates.insert(currentState);
        ++task->numberOfEncounteredStates;

        --remainingSteps;

        if (remainingSteps > 0) {
            currentState = State(nextState);
        } else {
            currentState = State(task->CPFs);
            remainingSteps = task->horizon;
            ++simCounter;
        }

        if (MathUtils::doubleIsGreater(t(), timeout)) {
            cout << "Stopping analysis after " << t << " seconds and "
                 << numSimulations << " simulations." << endl;
            break;
        }
    }
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

    task->numberOfUniqueEncounteredStates = encounteredStates.size();

    if (output) {
        cout << "    Creating training set..." << endl;
    }
    createTrainingSet(numStates);
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
}

bool actionDominates(
    ActionState const& action1, ActionState const& action2, CSP& csp) {
    // Check if there is a state where action1 yields a lower reward than
    // action2. If not, action1 dominates action2.
    csp.push();
    csp.assignActionVariables(action1.state, 0);
    csp.assignActionVariables(action2.state, 1);

    bool result = !csp.hasSolution();
    csp.pop();
    return result;
}

void TaskAnalyzer::determineTaskProperties() {
    RewardFunction* reward = task->rewardCPF;
    rewardFormulaAllowsRewardLockDetection =
        reward->isActionIndependent() && task->actionStates[0].isNOOP(task);

    if (reward->isActionIndependent()) {
        // The reward is not affected by the applied action, so we check if
        // there is an action that is always applicable (i.e., does not have any
        // precondition). If not, the first applicable action gives the final
        // reward.
        int actionIndex = -1;
        for (ActionState const& action : task->actionStates) {
            if (action.relevantSACs.empty()) {
                actionIndex = action.index;
                break;
            }
        }

        if (actionIndex != -1) {
            task->candidatesForOptimalFinalAction.push_back(actionIndex);
        }
    } else {
        // The reward is affected by the applied action, so we have to compare
        // all actions that are not dominated by another action in the
        // computation of the final reward
        CSP csp(task);
        csp.addActionVariables();

        // We look for a solution (i.e., a state) where the reward under the
        // second action is larger than the reward under the first. The first
        // action dominates the second if there is no such solution.
        LogicalExpression* reward = task->rewardCPF->formula;
        csp.addConstraint(
            reward->toZ3Formula(csp, 0) - reward->toZ3Formula(csp, 1) < 0);

        set<int> candidates;
        for (ActionState const& action : task->actionStates) {
            // Check if this action is dominated by an action that is already a
            // candidate.
            bool isDominatedByCandidate = false;
            for (int candidateIndex : candidates) {
                ActionState const& candidate =
                    task->actionStates[candidateIndex];
                if (!candidate.relevantSACs.empty()) {
                    // An action with preconditions cannot dominate another action
                    continue;
                }
                if (actionDominates(candidate, action, csp)) {
                    isDominatedByCandidate = true;
                    break;
                }
            }
            if (isDominatedByCandidate) {
                continue;
            }

            // Check if this action dominates actions in candidates
            if (!action.relevantSACs.empty()) {
                candidates.insert(action.index);
                // An action with preconditions cannot dominate another action
                continue;
            }

            set<int> remainingCandidates;
            remainingCandidates.insert(action.index);
            for (int candidateIndex : candidates) {
                ActionState const& candidate =
                    task->actionStates[candidateIndex];
                if (!actionDominates(action, candidate, csp)) {
                    remainingCandidates.insert(candidateIndex);
                }
            }
            swap(candidates, remainingCandidates);
        }
        task->candidatesForOptimalFinalAction.assign(
            candidates.begin(), candidates.end());
    }
}

void TaskAnalyzer::calculateMinAndMaxReward() const {
    // Compute the domain of the rewardCPF for deadend / goal detection (we only
    // need upper and lower bounds, but we use the same function here). If the
    // cachingType is vector, we can use the non-approximated values from the
    // precomputation further below.
    RewardFunction* reward = task->rewardCPF;
    double minVal = numeric_limits<double>::max();
    double maxVal = -numeric_limits<double>::max();
    if (reward->cachingType == "VECTOR") {
        // The reward has been precomputed, so we just need to find the
        // precomputed minimum and maximum
        for (double rew : reward->precomputedResults) {
            minVal = min(minVal, rew);
            maxVal = max(maxVal, rew);
        }
    } else {
        // The reward  is not cached in vectors, so it has not been precomputed.
        // We therefore approximate the minimum and maximum by using interval
        // arithmetic.
        int numCPFs = task->CPFs.size();

        vector<set<double>> domains(numCPFs);
        for (size_t index = 0; index < numCPFs; ++index) {
            vector<int> const& domain = task->CPFs[index]->domain;
            domains[index].insert(domain.begin(), domain.end());
        }

        for (ActionState const& action : task->actionStates) {
            double minRew = numeric_limits<double>::max();
            double maxRew = -numeric_limits<double>::max();
            reward->formula->calculateDomainAsInterval(
                domains, action, minRew, maxRew);
            minVal = min(minVal, minRew);
            maxVal = max(maxVal, maxRew);
        }
    }
    reward->domain.insert(minVal);
    reward->domain.insert(maxVal);
}

void TaskAnalyzer::analyzeStateAndApplyAction(State const& current, State& next,
                                              double& reward) const {
    if (!task->unreasonableActionInDeterminizationDetected) {
        // As long as we haven't found unreasonable actions in the
        // determinization we check if there are any in the current state
        detectUnreasonableActionsInDeterminization(current);
    }

    vector<int> applicableActions;
    set<PDState, PDState::PDStateSort> childStates;

    for (unsigned int actionIndex = 0; actionIndex < task->actionStates.size();
         ++actionIndex) {
        if (actionIsApplicable(task->actionStates[actionIndex], current)) {
            // This action is applicable
            PDState nxt(task->CPFs.size());
            for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
                task->CPFs[i]->formula->evaluateToPD(
                    nxt[i], current, task->actionStates[actionIndex]);
            }

            if (childStates.find(nxt) == childStates.end()) {
                // This action is reasonable
                childStates.insert(nxt);
                applicableActions.push_back(actionIndex);
            } else {
                // This action is not reasonable
                task->unreasonableActionDetected = true;
            }
        }
    }

    assert(!applicableActions.empty());

    // Check if this is a state with only one reasnoable applicable action
    if (applicableActions.size() == 1) {
        ++task->nonTerminalStatesWithUniqueAction;
        if (encounteredStates.find(current) == encounteredStates.end()) {
            ++task->uniqueNonTerminalStatesWithUniqueAction;
        }
    }

    // TODO: We could also simply sample the successor state that was calculated
    // in the reasonable action check above
    ActionState& randomAction =
        task->actionStates[applicableActions[std::rand() %
                                             applicableActions.size()]];
    for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
        task->CPFs[i]->formula->evaluate(next[i], current, randomAction);
    }
    task->rewardCPF->formula->evaluate(reward, current, randomAction);

    // Check if this is a reward lock
    if (rewardFormulaAllowsRewardLockDetection &&
        !task->rewardLockDetected &&
        (encounteredStates.find(current) == encounteredStates.end()) &&
        isARewardLock(current, reward)) {
        task->rewardLockDetected = true;
    }
}

void TaskAnalyzer::detectUnreasonableActionsInDeterminization(
    State const& current) const {
    set<State, State::StateSort> childStates;

    for (unsigned int actionIndex = 0; actionIndex < task->actionStates.size();
         ++actionIndex) {
        if (actionIsApplicable(task->actionStates[actionIndex], current)) {
            // This action is applicable
            State nxt(task->CPFs.size());
            for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
                if (task->CPFs[i]->determinization) {
                    task->CPFs[i]->determinization->evaluate(
                        nxt[i], current, task->actionStates[actionIndex]);
                } else {
                    task->CPFs[i]->formula->evaluate(
                        nxt[i], current, task->actionStates[actionIndex]);
                }
            }

            if (childStates.find(nxt) == childStates.end()) {
                // This action is reasonable
                childStates.insert(nxt);
            } else {
                // This action is not reasonable
                task->unreasonableActionInDeterminizationDetected = true;
                return;
            }
        }
    }
}

inline bool TaskAnalyzer::actionIsApplicable(ActionState const& action,
                                             State const& current) const {
    for (unsigned int precondIndex = 0;
         precondIndex < action.relevantSACs.size(); ++precondIndex) {
        double res = 0.0;
        action.relevantSACs[precondIndex]->formula->evaluate(res, current,
                                                             action);
        if (MathUtils::doubleIsEqual(res, 0.0)) {
            return false;
        }
    }
    return true;
}

inline bool TaskAnalyzer::isARewardLock(State const& current,
                                        double const& reward) const {
    if (MathUtils::doubleIsEqual(task->rewardCPF->getMinVal(), reward)) {
        KleeneState currentInKleene(current);
        return checkDeadEnd(currentInKleene);
    } else if (MathUtils::doubleIsEqual(task->rewardCPF->getMaxVal(), reward)) {
        KleeneState currentInKleene(current);
        return checkGoal(currentInKleene);
    }
    return false;
}

bool TaskAnalyzer::checkDeadEnd(KleeneState const& state) const {
    KleeneState mergedSuccs(task->CPFs.size());
    set<double> reward;

    for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
        task->CPFs[i]->formula->evaluateToKleene(mergedSuccs[i], state,
                                                 task->actionStates[0]);
    }
    task->rewardCPF->formula->evaluateToKleene(reward, state,
                                               task->actionStates[0]);

    // If reward is not minimal with certainty this is not a dead end
    if ((reward.size() != 1) ||
        !MathUtils::doubleIsEqual(*reward.begin(),
                                  task->rewardCPF->getMinVal())) {
        return false;
    }

    for (unsigned int actionIndex = 1; actionIndex < task->actionStates.size();
         ++actionIndex) {
        reward.clear();

        // Apply action actionIndex
        KleeneState succ(task->CPFs.size());
        for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
            task->CPFs[i]->formula->evaluateToKleene(
                succ[i], state, task->actionStates[actionIndex]);
        }
        task->rewardCPF->formula->evaluateToKleene(
            reward, state, task->actionStates[actionIndex]);

        // If reward is not minimal this is not a dead end
        if ((reward.size() != 1) ||
            !MathUtils::doubleIsEqual(*reward.begin(),
                                      task->rewardCPF->getMinVal())) {
            return false;
        }

        // Merge with previously computed successors
        mergedSuccs |= succ;
    }

    // Check if nothing changed, otherwise continue dead end check
    if ((mergedSuccs == state) || checkDeadEnd(mergedSuccs)) {
        return true;
    }
    return false;
}

// We underapproximate the set of goals, as we only consider those where
// applying goalTestActionIndex (which is always 0 at the moment) makes us stay
// in the reward lock.
bool TaskAnalyzer::checkGoal(KleeneState const& state) const {
    // Apply action goalTestActionIndex
    KleeneState succ(task->CPFs.size());
    set<double> reward;

    for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
        task->CPFs[i]->formula->evaluateToKleene(succ[i], state,
                                                 task->actionStates[0]);
    }
    task->rewardCPF->formula->evaluateToKleene(reward, state,
                                               task->actionStates[0]);

    // If reward is not maximal with certainty this is not a goal
    if ((reward.size() > 1) ||
        !MathUtils::doubleIsEqual(task->rewardCPF->getMaxVal(),
                                  *reward.begin())) {
        return false;
    }

    // Add parent to successor
    succ |= state;

    // Check if nothing changed, otherwise continue goal check
    if ((succ == state) || checkGoal(succ)) {
        return true;
    }
    return false;
}

void TaskAnalyzer::createTrainingSet(int const& numberOfStates) {
    cout << "Creating training set with " << encounteredStates.size()
         << " candidates." << endl;
    if (encounteredStates.size() < numberOfStates) {
        task->trainingSet = encounteredStates;
    } else {
        // We want the initial state to be part of the training set
        State initialState = State(task->CPFs);
        task->trainingSet.insert(initialState);
        assert(encounteredStates.find(initialState) != encounteredStates.end());
        encounteredStates.erase(initialState);

        // Then include states at random until the size of the trainingSet is as
        // desired
        while (task->trainingSet.size() != numberOfStates) {
            int randNum = (std::rand() % encounteredStates.size());
            set<State, State::StateSort>::const_iterator it =
                encounteredStates.begin();
            std::advance(it, randNum);
            task->trainingSet.insert(*it);
            encounteredStates.erase(it);
        }
    }
}
