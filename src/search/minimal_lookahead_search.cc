#include "minimal_lookahead_search.h"

using namespace std;

MinimalLookaheadSearch::HashMap MinimalLookaheadSearch::rewardCache;

MinimalLookaheadSearch::MinimalLookaheadSearch()
    : DeterministicSearchEngine("MLS"), numberOfRuns(0), cacheHits(0) {
    if (rewardCache.bucket_count() < 520241) {
        rewardCache.reserve(520241);
    }
}

void MinimalLookaheadSearch::estimateQValue(State const& state, int actionIndex,
                                            double& qValue) {
    HashMap::iterator it = rewardCache.find(state);

    if (it != rewardCache.end() &&
        !MathUtils::doubleIsMinusInfinity(it->second[actionIndex])) {
        ++cacheHits;
        qValue = it->second[actionIndex] * (double)state.stepsToGo();
        ;
    } else {
        // Apply the action to state
        calcReward(state, actionIndex, qValue);

        if (rewardCPF->isActionIndependent() ||
            (actionStates[0].scheduledActionFluents.empty() &&
             actionStates[0].actionPreconditions.empty())) {
            // Caculate the successor state if the action with index actionIndex
            // is applied, and use noop to calculate the reward in the next
            // state. This increases the informativeness of the reward that can
            // be achieved.
            State next;
            calcSuccessorState(state, actionIndex, next);

            double reward;
            calcReward(next, 0, reward);
            qValue = (qValue + reward) / 2.0;
        }

        if (cachingEnabled) {
            if (it == rewardCache.end()) {
                rewardCache[state] =
                    vector<double>(SearchEngine::numberOfActions,
                                   -std::numeric_limits<double>::max());
            }
            rewardCache[state][actionIndex] = qValue;
        }
        qValue *= (double)state.stepsToGo();

        ++numberOfRuns;
    }
}

void MinimalLookaheadSearch::estimateQValues(State const& state,
                                             vector<int> const& actionsToExpand,
                                             vector<double>& qValues) {
    HashMap::iterator it = rewardCache.find(state);

    if (it != rewardCache.end()) {
        ++cacheHits;
        assert(qValues.size() == it->second.size());
        for (size_t index = 0; index < qValues.size(); ++index) {
            if (actionsToExpand[index] == index) {
                qValues[index] = it->second[index] * (double)state.stepsToGo();
            } else {
                qValues[index] = -std::numeric_limits<double>::max();
            }
        }
    } else {
        if (rewardCPF->isActionIndependent()) {
            // Calculate the reward in state. It doesn't matter which action we
            // use for this, since there is no action fluent in the reward
            // formula anyway (it therefore doesn't even matter if the used
            // action is applicable or not)
            double reward = 0.0;
            calcReward(state, 0, reward);

            for (size_t index = 0; index < actionsToExpand.size(); ++index) {
                if (actionsToExpand[index] == index) {
                    // Calculate the successor state given this action is
                    // applied (i.e., the action matters here!)
                    State next;
                    calcSuccessorState(state, index, next);

                    // Now that we have the successor state, we can (again) use
                    // any action to calculate the reward of the next state.
                    double reward2;
                    calcReward(next, 0, reward2);
                    qValues[index] = (reward + reward2) / 2.0;
                }
            }
        } else if (actionStates[0].scheduledActionFluents.empty() &&
                   actionStates[0].actionPreconditions.empty()) {
            // There is an action fluent in the reward (and the reward in state
            // hence depends on the applied action), but it is often the case
            // that only the "bad" part of applying an action is in this result,
            // while the positive effect of the action is not. Since noop is
            // always applicable in this task, we apply in the next state to
            // account for those positive effects.
            for (size_t index = 0; index < actionsToExpand.size(); ++index) {
                if (actionsToExpand[index] == index) {
                    State next;
                    double reward;
                    calcStateTransition(state, index, next, reward);

                    // Use noop to calculate the reward in the next state.
                    double reward2;
                    calcReward(next, 0, reward2);
                    qValues[index] = (reward + reward2) / 2.0;
                }
            }
        } else {
            // Apply all actions to state
            for (size_t index = 0; index < actionsToExpand.size(); ++index) {
                if (actionsToExpand[index] == index) {
                    calcReward(state, index, qValues[index]);
                }
            }
        }

        if (cachingEnabled) {
            rewardCache[state] = qValues;
        }

        for (size_t index = 0; index < qValues.size(); ++index) {
            if (actionsToExpand[index] == index) {
                qValues[index] *= (double)state.stepsToGo();
            }
        }

        ++numberOfRuns;
    }
}

void MinimalLookaheadSearch::printStats(ostream& out,
                                        bool const& /*printRoundStats*/,
                                        string indent) const {
    out << indent << "Cache hits: " << cacheHits << " (in " << numberOfRuns
        << " runs)" << endl;
}
