#include "task_analyzer.h"

#include "csp.h"
#include "evaluatables.h"
#include "rddl.h"

#include "utils/math.h"
#include "utils/timer.h"
#include "utils/system.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

using namespace std;

namespace prost::parser {
void TaskAnalyzer::analyzeTask(int numStates, int numSimulations,
                               double timeout, bool output) {
    utils::Timer t;
    if (output) {
        cout << "    Determining task properties..." << endl;
    }
    determineTaskProperties();
    if (output) {
        cout << "    ...finished (" << t() << ")" << endl;
    }
    t.reset();

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
    performRandomWalks(numSimulations, timeout);
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
    if (task->rewardCPF->isActionIndependent()) {
        // The reward is not affected by the applied action, so we check if
        // there is an action that is always applicable (i.e., does not have any
        // precondition). If not, the first applicable action gives the final
        // reward.
        auto noPreconds = [](ActionState const& action) {
            return action.relevantSACs.empty();
        };
        auto noPrecondIt = std::find_if(task->actionStates.begin(),
                                        task->actionStates.end(), noPreconds);
        if (noPrecondIt != task->actionStates.end()) {
            task->candidatesForOptimalFinalAction.push_back(noPrecondIt->index);
        }
    } else {
        // The reward is affected by the applied action, so we have to compare
        // all actions that are not dominated by another action in the
        // computation of the final reward
        task->candidatesForOptimalFinalAction = determineDominantActions();
    }

    // To determine if a state is a goal, we have to check if applying the
    // candidate for the optimal final action that yields the highest reward
    // leads to a state that is still in the goal zone. As it is expensive if
    // there is more than just one such candidate, we disable reward lock
    // detection if there is more than one candidate.
    if (task->candidatesForOptimalFinalAction.size() == 1) {
        finalActionIndex = task->candidatesForOptimalFinalAction[0];
    }
}

namespace {
bool actionDominates(ActionState const& action1, ActionState const& action2,
                     RDDLTaskCSP& csp) {
    if (!action1.relevantSACs.empty()) {
        // An action with preconditions cannot dominate another action
        return false;
    }
    // Check if there is a state where action1 yields a lower reward than
    // action2. If not, action1 dominates action2.
    csp.push();
    csp.assignActionVarSet(action1.state, 0);
    csp.assignActionVarSet(action2.state, 1);

    bool result = !csp.hasSolution();
    csp.pop();
    return result;
}
} // namespace

vector<int> TaskAnalyzer::determineDominantActions() const {
    RDDLTaskCSP csp(task);
    csp.addActionVarSet();

    // We look for a solution (i.e., a state) where the reward under the
    // second action is larger than the reward under the first. The first
    // action dominates the second if there is no such solution.
    LogicalExpression* reward = task->rewardCPF->formula;
    csp.addConstraint(
        reward->toZ3Formula(csp, 0) - reward->toZ3Formula(csp, 1) < 0);

    set<int> candidates;
    for (ActionState const& action : task->actionStates) {
        // Add action if it is not dominated by a candidate
        auto dominatesAction = [&](int other) {
            return actionDominates(task->actionStates[other], action, csp);
        };
        if (any_of(candidates.begin(), candidates.end(), dominatesAction)) {
            continue;
        }

        // Remove candidates that are dominated by action
        set<int> remainingCandidates;
        remainingCandidates.insert(action.index);
        for (int candidateIndex : candidates) {
            ActionState const& candidate = task->actionStates[candidateIndex];
            if (!actionDominates(action, candidate, csp)) {
                remainingCandidates.insert(candidateIndex);
            }
        }
        swap(candidates, remainingCandidates);
    }
    return vector<int>(candidates.begin(), candidates.end());
}

void TaskAnalyzer::calculateMinAndMaxReward() const {
    // Compute the domain of the rewardCPF for deadend / goal detection (we only
    // need upper and lower bounds, but we use the same function here). If the
    // cachingType is vector, we can use the non-approximated values from the
    // precomputation further below.
    if (task->rewardCPF->cachingType == "VECTOR") {
        // The reward has been precomputed, so we use the precomputed values
        vector<double> const& values = task->rewardCPF->precomputedResults;
        task->rewardCPF->minValue = *min_element(values.begin(), values.end());
        task->rewardCPF->maxValue = *max_element(values.begin(), values.end());
    } else {
        task->rewardCPF->minValue = numeric_limits<double>::max();
        task->rewardCPF->maxValue = -numeric_limits<double>::max();
        // The reward  is not cached in vectors, so it has not been precomputed.
        // We therefore approximate the minimum and maximum by using interval
        // arithmetic.
        vector<set<double>> domains(task->CPFs.size());
        for (size_t i = 0; i < task->CPFs.size(); ++i) {
            vector<int> const& domain = task->CPFs[i]->domain;
            domains[i].insert(domain.begin(), domain.end());
        }

        for (ActionState const& action : task->actionStates) {
            double minRew = numeric_limits<double>::max();
            double maxRew = -numeric_limits<double>::max();
            task->rewardCPF->formula->calculateDomainAsInterval(domains, action,
                                                                minRew, maxRew);
            task->rewardCPF->minValue = min(task->rewardCPF->minValue, minRew);
            task->rewardCPF->maxValue = max(task->rewardCPF->maxValue, maxRew);
        }
    }
}

void TaskAnalyzer::performRandomWalks(int numSimulations, double timeout) {
    utils::Timer t;
    State current(task->CPFs);
    int remainingSteps = task->horizon;

    for (int simCounter = 0; simCounter < numSimulations;) {
        State next(task->CPFs.size());
        double reward = 0.0;
        analyzeStateAndApplyAction(current, next, reward);

        encounteredStates.insert(current);
        ++task->numberOfEncounteredStates;

        --remainingSteps;

        if (remainingSteps > 0) {
            current = State(next);
        } else {
            current = State(task->CPFs);
            remainingSteps = task->horizon;
            ++simCounter;
        }

        if (utils::doubleIsGreater(t(), timeout)) {
            cout << "Stopping analysis after " << t << " seconds and "
                 << numSimulations << " simulations." << endl;
            break;
        }
    }
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

    if (applicableActions.empty()) {
        stateWithoutApplicableActionsDetected(current);
    }

    // Check if this is a state with only one reasonable applicable action
    if (applicableActions.size() == 1) {
        ++task->nonTerminalStatesWithUniqueAction;
        if (encounteredStates.find(current) == encounteredStates.end()) {
            ++task->uniqueNonTerminalStatesWithUniqueAction;
        }
    }

    ActionState& randomAction =
        task->actionStates[applicableActions[std::rand() %
                                             applicableActions.size()]];
    for (unsigned int i = 0; i < task->CPFs.size(); ++i) {
        task->CPFs[i]->formula->evaluate(next[i], current, randomAction);
    }
    task->rewardCPF->formula->evaluate(reward, current, randomAction);

    // Check if this is a reward lock
    if ((finalActionIndex >= 0) && !task->rewardLockDetected &&
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
        if (utils::doubleIsEqual(res, 0.0)) {
            return false;
        }
    }
    return true;
}

inline bool TaskAnalyzer::isARewardLock(State const& current,
                                        double const& reward) const {
    if (utils::doubleIsEqual(task->rewardCPF->minValue, reward)) {
        KleeneState currentInKleene(current);
        return checkDeadEnd(currentInKleene);
    } else if (utils::doubleIsEqual(task->rewardCPF->maxValue, reward)) {
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
        !utils::doubleIsEqual(*reward.begin(), task->rewardCPF->minValue)) {
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
            !utils::doubleIsEqual(*reward.begin(), task->rewardCPF->minValue)) {
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
        task->CPFs[i]->formula->evaluateToKleene(
            succ[i], state, task->actionStates[finalActionIndex]);
    }
    task->rewardCPF->formula->evaluateToKleene(
        reward, state, task->actionStates[finalActionIndex]);

    // If reward is not maximal with certainty this is not a goal
    if ((reward.size() > 1) ||
        !utils::doubleIsEqual(task->rewardCPF->maxValue, *reward.begin())) {
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

void TaskAnalyzer::stateWithoutApplicableActionsDetected(
    State const& state) const{
    cout << endl;
    stringstream out;
    out << "Prost cannot deal with planning tasks where a state without "
           "applicable actions can be reached." << endl << "The following such "
           "state was encountered: " << endl << state.toString(task) << endl
        << "The following preconditions of each action are violated:" << endl;
    for (ActionState const& action : task->actionStates) {
        out << action.toString(task) << ": " << endl;
        for (ActionPrecondition* precond : action.relevantSACs) {
            double res = 0.0;
            precond->formula->evaluate(res, state, action);
            if (utils::doubleIsEqual(res, 0.0)) {
                out << "  ";
                precond->formula->prettyPrint(out);
                out << endl;
            }
        }
    }
    utils::abort(out.str());
}

} // namespace prost::parser
