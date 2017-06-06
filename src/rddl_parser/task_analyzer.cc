#include "task_analyzer.h"

#include "evaluatables.h"
#include "rddl.h"

#include "utils/system_utils.h"
#include "utils/timer.h"

#include <cstdlib>
#include <iostream>

using namespace std;

void TaskAnalyzer::analyzeTask(int numStates, int numSimulations) {
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
    }

    task->numberOfUniqueEncounteredStates = encounteredStates.size();

    createTrainingSet(numStates);
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
