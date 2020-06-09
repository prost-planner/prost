#include "task_analyzer.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/system_utils.h"
#include "utils/timer.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

using namespace std;

void TaskAnalyzer::analyzeTask(int numStates, int numSimulations, double timeout, bool output) {
    Timer t;

    // Determine some non-trivial properties
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

void TaskAnalyzer::determineTaskProperties() {
    // Determine if there is a single action that could be goal maintaining,
    // i.e., that could always yield the maximal reward.
    // TODO: We could use an action that contains as many positive and no
    //  negative occuring fluents as possible, but we should first figure out
    //  if reward lock detection still pays off.
    RewardFunction* reward = task->rewardCPF;

    task->rewardFormulaAllowsRewardLockDetection =
        reward->positiveActionDependencies.empty() &&
        task->actionStates[0].isNOOP(task);


    // TODO: This is the only place where the positive action dependencies are
    //  used, and they do no longer work the way they did before the
    //  introduction of FDR action fluents. What we need to figure out here is
    //  the question if the reward of action a is always larger than or equal
    //  to the reward of action a', and we should be able to answer this with
    //  the use of z3 by chceking if there is a solution (i.e., a state) where
    //  R(s,a) - R(s,a') < 0. The only other place where the positive action
    //  dependencies are used is the code directly above this comment.

    // Determine the set of actions that must be applied in the last step (the
    // final reward only depends on the current state and the action that is
    // applied but not the successor state, so we can determine a set of actions
    // that could be optimal as last action beforehand).
    if (reward->positiveActionDependencies.empty() &&
        task->actionStates[0].isNOOP(task) &&
        task->actionStates[0].relevantSACs.empty()) {
        // The first action is noop, noop is always applicable and action
        // fluents occur in the reward only as costs -> noop is always optimal
        // as final action
        // TODO: I cannot guarantee that the check if actions occur only as
        //  costs is correct. This should be verified at some point.
        task->finalRewardCalculationMethod = "NOOP";
    } else if (reward->isActionIndependent()) {
        // The reward formula does not contain any action fluents -> all actions
        // yield the same reward, so any action that is applicable is optimal
        task->finalRewardCalculationMethod = "FIRST_APPLICABLE";
    } else {
        task->finalRewardCalculationMethod = "BEST_OF_CANDIDATE_SET";
        // Determine the actions that suffice to be applied in the final step.
        for (ActionState const& action : task->actionStates) {
            if (!actionStateIsDominated(action)) {
                addDominantState(action);
            }
        }
    }
}

void TaskAnalyzer::addDominantState(ActionState const& action) const {
    vector<int> candidates;
    for (int actionIndex : task->candidatesForOptimalFinalAction) {
        ActionState const& candidate = task->actionStates[actionIndex];
        if (!actionStateDominates(action, candidate)) {
            candidates.push_back(actionIndex);
        }
    }
    candidates.push_back(action.index);
    swap(candidates, task->candidatesForOptimalFinalAction);
}

bool TaskAnalyzer::actionStateIsDominated(ActionState const& action) const {
    for (int actionIndex : task->candidatesForOptimalFinalAction) {
        ActionState const& candidate = task->actionStates[actionIndex];
        if (actionStateDominates(candidate, action)) {
            return true;
        }
    }
    return false;
}

bool TaskAnalyzer::actionStateDominates(ActionState const& lhs,
                                        ActionState const& rhs) const {
    // An action state with preconditions cannot dominate another action state
    if (!lhs.relevantSACs.empty()) {
        return false;
    }

    set<ActionFluent*>& positiveInReward =
        task->rewardCPF->positiveActionDependencies;
    set<ActionFluent*>& negativeInReward =
        task->rewardCPF->negativeActionDependencies;

    // Determine all fluents of both action states that influence the reward
    // positively or negatively
    set<ActionFluent*> lhsPos;
    set<ActionFluent*> lhsNeg;
    for (ActionFluent* af : lhs.scheduledActionFluents) {
        if (positiveInReward.find(af) != positiveInReward.end()) {
            lhsPos.insert(af);
        }
        if (negativeInReward.find(af) != negativeInReward.end()) {
            lhsNeg.insert(af);
        }
    }

    set<ActionFluent*> rhsPos;
    set<ActionFluent*> rhsNeg;
    for (ActionFluent* af : rhs.scheduledActionFluents) {
        if (positiveInReward.find(af) != positiveInReward.end()) {
            rhsPos.insert(af);
        }
        if (negativeInReward.find(af) != negativeInReward.end()) {
            rhsNeg.insert(af);
        }
    }

    // Action state lhs dominates rhs if lhs contains all action fluents that
    // influence the reward positively of rhs, and if rhs contains all action
    // fluents that influence the reward negatively of lhs
    for (ActionFluent* af : rhsPos) {
        if (lhsPos.find(af) == lhsPos.end()) {
            return false;
        }
    }
    for (ActionFluent* af : lhsNeg) {
        if (rhsNeg.find(af) == rhsNeg.end()) {
            return false;
        }
    }
    return true;
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
            domains[index] = task->CPFs[index]->domain;
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
    if (task->rewardFormulaAllowsRewardLockDetection &&
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
