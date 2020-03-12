#include "thts.h"

#include "action_selection.h"
#include "backup_function.h"
#include "initializer.h"
#include "outcome_selection.h"
#include "recommendation_function.h"

#include "utils/logger.h"
#include "utils/system_utils.h"

#include <sstream>

std::string SearchNode::toString() const {
    std::stringstream ss;
    if (solved) {
        ss << "SOLVED with: ";
    }
    ss << getExpectedRewardEstimate() << " (in "
       << numberOfVisits << " real visits)";
    return ss.str();
}

THTS::THTS(std::string _name)
    : ProbabilisticSearchEngine(_name),
      actionSelection(nullptr),
      outcomeSelection(nullptr),
      backupFunction(nullptr),
      initializer(nullptr),
      recommendationFunction(nullptr),
      currentRootNode(nullptr),
      chosenOutcome(nullptr),
      tipNodeOfTrial(nullptr),
      states(SearchEngine::horizon + 1),
      stepsToGoInCurrentState(SearchEngine::horizon),
      stepsToGoInNextState(SearchEngine::horizon - 1),
      appliedActionIndex(-1),
      trialReward(0.0),
      currentTrial(0),
      initializedDecisionNodes(0),
      lastUsedNodePoolIndex(0),
      terminationMethod(THTS::TIME),
      maxNumberOfTrials(0),
      numberOfNewDecisionNodesPerTrial(1),
      cacheHits(0),
      uniquePolicyDueToLastAction(false),
      uniquePolicyDueToRewardLock(false),
      uniquePolicyDueToPreconds(false),
      stepsToGoInFirstSolvedState(-1),
      numTrialsInInitialState(0),
      numSearchNodesInInitialState(0),
      numRewardLockStates(0),
      numSingleApplicableActionStates(0) {
    setMaxNumberOfNodes(24000000);
    setTimeout(1.0);
    setRecommendationFunction(new ExpectedBestArmRecommendation(this));
}

bool THTS::setValueFromString(std::string& param, std::string& value) {
    // Check if this parameter encodes an ingredient
    if (param == "-act") {
        setActionSelection(ActionSelection::fromString(value, this));

        return true;
    } else if (param == "-out") {
        setOutcomeSelection(OutcomeSelection::fromString(value, this));

        return true;
    } else if (param == "-backup") {
        setBackupFunction(BackupFunction::fromString(value, this));

        return true;
    } else if (param == "-init") {
        setInitializer(Initializer::fromString(value, this));
        return true;
    } else if (param == "-rec") {
        setRecommendationFunction(
            RecommendationFunction::fromString(value, this));
        return true;
    }

    if (param == "-T") {
        if (value == "TIME") {
            setTerminationMethod(THTS::TIME);
            return true;
        } else if (value == "TRIALS") {
            setTerminationMethod(THTS::NUMBER_OF_TRIALS);
            return true;
        } else if (value == "TIME_AND_TRIALS") {
            setTerminationMethod(THTS::TIME_AND_NUMBER_OF_TRIALS);
            return true;
        } else {
            return false;
        }
    } else if (param == "-r") {
        setMaxNumberOfTrials(atoi(value.c_str()));
        return true;
    } else if (param == "-ndn") {
        if (value == "H") {
            setNumberOfNewDecisionNodesPerTrial(SearchEngine::horizon);
        } else {
            setNumberOfNewDecisionNodesPerTrial(atoi(value.c_str()));
        }
        return true;
    } else if (param == "-node-limit") {
        setMaxNumberOfNodes(atoi(value.c_str()));
        return true;
    }

    return SearchEngine::setValueFromString(param, value);
}

void THTS::setActionSelection(ActionSelection* _actionSelection) {
    if (actionSelection) {
        delete actionSelection;
    }
    actionSelection = _actionSelection;
}

void THTS::setOutcomeSelection(OutcomeSelection* _outcomeSelection) {
    if (outcomeSelection) {
        delete outcomeSelection;
    }
    outcomeSelection = _outcomeSelection;
}

void THTS::setBackupFunction(BackupFunction* _backupFunction) {
    if (backupFunction) {
        delete backupFunction;
    }
    backupFunction = _backupFunction;
}

void THTS::setInitializer(Initializer* _initializer) {
    if (initializer) {
        delete initializer;
    }
    initializer = _initializer;
}

void THTS::setRecommendationFunction(
    RecommendationFunction* _recommendationFunction) {
    if (recommendationFunction) {
        delete recommendationFunction;
    }
    recommendationFunction = _recommendationFunction;
}

void THTS::disableCaching() {
    actionSelection->disableCaching();
    outcomeSelection->disableCaching();
    backupFunction->disableCaching();
    initializer->disableCaching();
    recommendationFunction->disableCaching();
    SearchEngine::disableCaching();
}

void THTS::initSession() {
    // All ingredients must have been specified
    if (!actionSelection || !outcomeSelection || !backupFunction ||
        !initializer || !recommendationFunction) {
        SystemUtils::abort(
            "Action selection, outcome selection, backup "
            "function, initializer, and recommendation function "
            "must be defined in a THTS search engine!");
    }

    actionSelection->initSession();
    outcomeSelection->initSession();
    backupFunction->initSession();
    initializer->initSession();
    recommendationFunction->initSession();
}

void THTS::initRound() {
    // Reset per round statistics
    stepsToGoInFirstSolvedState = -1;
    numTrialsInInitialState = 0;
    numSearchNodesInInitialState = 0;
    numRewardLockStates = 0;
    numSingleApplicableActionStates = 0;

    // Notify ingredients of new round
    actionSelection->initRound();
    outcomeSelection->initRound();
    backupFunction->initRound();
    initializer->initRound();
}

void THTS::finishRound() {
    // Notify ingredients of end of round
    actionSelection->finishRound();
    outcomeSelection->finishRound();
    backupFunction->finishRound();
    initializer->finishRound();
}

void THTS::initStep(State const& current) {
    PDState rootState(current);
    // Adjust maximal search depth and set root state
    if (rootState.stepsToGo() > maxSearchDepth) {
        maxSearchDepthForThisStep = maxSearchDepth;
        states[maxSearchDepthForThisStep].setTo(rootState);
        states[maxSearchDepthForThisStep].stepsToGo() =
            maxSearchDepthForThisStep;
    } else {
        maxSearchDepthForThisStep = rootState.stepsToGo();
        states[maxSearchDepthForThisStep].setTo(rootState);
    }
    assert(states[maxSearchDepthForThisStep].stepsToGo() ==
           maxSearchDepthForThisStep);

    stepsToGoInCurrentState = maxSearchDepthForThisStep;
    stepsToGoInNextState = maxSearchDepthForThisStep - 1;
    states[stepsToGoInNextState].reset(stepsToGoInNextState);

    currentTrial = 0;

    // Reset per step statistics
    cacheHits = 0;
    lastSearchTime = 0.0;
    uniquePolicyDueToLastAction = false;
    uniquePolicyDueToRewardLock = false;
    uniquePolicyDueToPreconds = false;

    // Create root node
    currentRootNode = createRootNode();

    // Notify ingredients of new step
    actionSelection->initStep(current);
    outcomeSelection->initStep();
    backupFunction->initStep();
    initializer->initStep(current);
}

void THTS::finishStep() {
    if (uniquePolicyDueToRewardLock) {
        ++numRewardLockStates;
    } else if (uniquePolicyDueToPreconds) {
        ++numSingleApplicableActionStates;
    }

    actionSelection->finishStep();
    outcomeSelection->finishStep();
    backupFunction->finishStep();
    initializer->finishStep();
}

inline void THTS::initTrial() {
    // Reset states and steps-to-go counter
    stepsToGoInCurrentState = maxSearchDepthForThisStep;
    stepsToGoInNextState = maxSearchDepthForThisStep - 1;
    states[stepsToGoInNextState].reset(stepsToGoInNextState);

    // Reset trial dependent variables
    initializedDecisionNodes = 0;
    trialReward = 0.0;
    tipNodeOfTrial = nullptr;

    // Notify ingredients of new trial
    actionSelection->initTrial();
    outcomeSelection->initTrial();
    backupFunction->initTrial();
    initializer->initTrial();
}

inline void THTS::initTrialStep() {
    --stepsToGoInCurrentState;
    --stepsToGoInNextState;
    states[stepsToGoInNextState].reset(stepsToGoInNextState);
}

void THTS::estimateBestActions(State const& _rootState,
                               std::vector<int>& bestActions) {
    assert(bestActions.empty());
    stopwatch.reset();

    int stepsToGo = _rootState.stepsToGo();

    // Check if there is an obviously optimal policy (as, e.g., in the last step
    // or in a reward lock)
    int uniquePolicyOpIndex = getUniquePolicy();
    if (uniquePolicyOpIndex != -1) {
        bestActions.push_back(uniquePolicyOpIndex);
        currentRootNode = nullptr;

        // Update statistics
        if ((stepsToGoInFirstSolvedState == -1) &&
            (uniquePolicyDueToLastAction || uniquePolicyDueToRewardLock)) {
            stepsToGoInFirstSolvedState = stepsToGo;
        }

        return;
    }

    // Perform trials until some termination criterion is fullfilled
    while (moreTrials()) {
        // Logger::logSeparator(Verbosity::DEBUG);
        // Logger::logLine("TRIAL " + std::to_string(currentTrial+1),
        //             Verbosity::DEBUG);

        visitDecisionNode(currentRootNode);
        ++currentTrial;

        // for(unsigned int i = 0; i < currentRootNode->children.size(); ++i) {
        //     if (currentRootNode->children[i]) {
        //         Logger::logLine(SearchEngine::actionStates[i].toCompactString() +
        //                     ": " + currentRootNode->children[i]->toString(),
        //                     Verbosity::DEBUG);
        //     }
        // }
        // assert(currentTrial != 100);
    }

    recommendationFunction->recommend(currentRootNode, bestActions);
    assert(!bestActions.empty());

    // Update statistics
    if (currentRootNode->solved && (stepsToGoInFirstSolvedState == -1)) {
        // TODO: This is the first root state that was solved, so everything
        //  that could happen in the future is also solved. We should (at least
        //  in this case) make sure that we keep the tree and simply follow the
        //  optimal policy.
        stepsToGoInFirstSolvedState = stepsToGo;
    }

    if (stepsToGo == SearchEngine::horizon) {
        numTrialsInInitialState = currentTrial;
        numSearchNodesInInitialState = lastUsedNodePoolIndex;
    }

    // Memorize search time
    lastSearchTime = stopwatch();
}

bool THTS::moreTrials() {
    // Check memory constraints and solvedness
    if (currentRootNode->solved ||
        (lastUsedNodePoolIndex >= maxNumberOfNodes)) {
        return false;
    }

    if (currentTrial == 0) {
        return true;
    }

    // Check selected termination criterion
    switch (terminationMethod) {
    case THTS::TIME:
        if (MathUtils::doubleIsGreater(stopwatch(), timeout)) {
            return false;
        }
        break;
    case THTS::NUMBER_OF_TRIALS:
        if (currentTrial == maxNumberOfTrials) {
            return false;
        }
        break;
    case THTS::TIME_AND_NUMBER_OF_TRIALS:
        if (MathUtils::doubleIsGreater(stopwatch(), timeout) ||
            (currentTrial == maxNumberOfTrials)) {
            return false;
        }
        break;
    }

    return true;
}

void THTS::visitDecisionNode(SearchNode* node) {
    if (node == currentRootNode) {
        initTrial();
    } else {
        // Continue trial (i.e., set next state to be the current)
        initTrialStep();

        // Check if there is a "special" reason to stop this trial (currently,
        // this is the case if the state value of the current state is cached,
        // if it is a reward lock or if there is only one step left).
        if (currentStateIsSolved(node)) {
            if (!tipNodeOfTrial) {
                tipNodeOfTrial = node;
            }
            return;
        }
    }

    // Initialize node if necessary
    if (!node->initialized) {
        if (!tipNodeOfTrial) {
            tipNodeOfTrial = node;
        }

        initializer->initialize(node, states[stepsToGoInCurrentState]);

        if (node != currentRootNode) {
            ++initializedDecisionNodes;
        }
    }

    // Logger::logLine("Current state is:", Verbosity::DEBUG);
    // Logger::logLine(states[stepsToGoInCurrentState].toString(),
    //             Verbosity::DEBUG);
    // Logger::logLine("Reward is " + std::to_string(node->immediateReward),
    //             Verbosity::DEBUG);

    // Determine if we continue with this trial
    if (continueTrial(node)) {
        // Select the action that is simulated
        appliedActionIndex = actionSelection->selectAction(node);
        assert(node->children[appliedActionIndex]);
        assert(!node->children[appliedActionIndex]->solved);

        // Logger::logLine("Chosen action is: " +
        //                 SearchEngine::actionStates[appliedActionIndex].toCompactString(),
        //                 Verbosity::DEBUG);

        // Sample successor state
        calcSuccessorState(states[stepsToGoInCurrentState], appliedActionIndex,
                           states[stepsToGoInNextState]);

        // Logger::logLine("Sampled PDState is " +
        //                 states[stepsToGoInNextState].toCompactString(),
        //                 Verbosity::DEBUG);

        lastProbabilisticVarIndex = -1;
        for (unsigned int i = 0; i < State::numberOfProbabilisticStateFluents;
             ++i) {
            if (states[stepsToGoInNextState]
                    .probabilisticStateFluentAsPD(i)
                    .isDeterministic()) {
                states[stepsToGoInNextState].probabilisticStateFluent(i) =
                    states[stepsToGoInNextState]
                        .probabilisticStateFluentAsPD(i)
                        .values[0];
            } else {
                lastProbabilisticVarIndex = i;
            }
        }

        // Start outcome selection with the first probabilistic variable
        chanceNodeVarIndex = 0;

        // Continue trial with chance nodes
        if (lastProbabilisticVarIndex < 0) {
            visitDummyChanceNode(node->children[appliedActionIndex]);
        } else {
            visitChanceNode(node->children[appliedActionIndex]);
        }

        // Backup this node
        backupFunction->backupDecisionNode(node);
        trialReward += node->immediateReward;

        // If the backup function labeled the node as solved, we store the
        // result for the associated state in case we encounter it somewhere
        // else in the tree in the future
        if (node->solved) {
            if (cachingEnabled &&
                ProbabilisticSearchEngine::stateValueCache.find(
                    states[node->stepsToGo]) ==
                    ProbabilisticSearchEngine::stateValueCache.end()) {
                ProbabilisticSearchEngine::stateValueCache
                    [states[node->stepsToGo]] =
                        node->getExpectedFutureRewardEstimate();
            }
        }
    } else {
        // The trial is finished
        trialReward = node->getExpectedRewardEstimate();
    }
}

bool THTS::currentStateIsSolved(SearchNode* node) {
    if (stepsToGoInCurrentState == 1) {
        // This node is a leaf (there is still a last decision, though, but that
        // is taken care of by calcOptimalFinalReward)

        calcOptimalFinalReward(states[1], trialReward);
        backupFunction->backupDecisionNodeLeaf(node, trialReward);
        trialReward += node->immediateReward;

        return true;
    } else if (ProbabilisticSearchEngine::stateValueCache.find(
                   states[stepsToGoInCurrentState]) !=
               ProbabilisticSearchEngine::stateValueCache.end()) {
        // This state has already been solved before
        trialReward = ProbabilisticSearchEngine::stateValueCache
            [states[stepsToGoInCurrentState]];
        backupFunction->backupDecisionNodeLeaf(node, trialReward);
        trialReward += node->immediateReward;

        ++cacheHits;
        return true;
    } else if (node->children.empty() &&
               isARewardLock(states[stepsToGoInCurrentState])) {
        // This state is a reward lock, i.e. a goal or a state that is such that
        // no matter which action is applied we'll always get the same reward

        calcReward(states[stepsToGoInCurrentState], 0, trialReward);
        trialReward *= stepsToGoInCurrentState;
        backupFunction->backupDecisionNodeLeaf(node, trialReward);
        trialReward += node->immediateReward;

        if (cachingEnabled) {
            assert(ProbabilisticSearchEngine::stateValueCache.find(
                       states[stepsToGoInCurrentState]) ==
                   ProbabilisticSearchEngine::stateValueCache.end());
            ProbabilisticSearchEngine::stateValueCache
                [states[stepsToGoInCurrentState]] =
                    node->getExpectedFutureRewardEstimate();
        }
        return true;
    }
    return false;
}

void THTS::visitChanceNode(SearchNode* node) {
    while (states[stepsToGoInNextState]
               .probabilisticStateFluentAsPD(chanceNodeVarIndex)
               .isDeterministic()) {
        ++chanceNodeVarIndex;
    }

    chosenOutcome = outcomeSelection->selectOutcome(
        node, states[stepsToGoInNextState], chanceNodeVarIndex,
        lastProbabilisticVarIndex);

    if (chanceNodeVarIndex == lastProbabilisticVarIndex) {
        State::calcStateFluentHashKeys(states[stepsToGoInNextState]);
        State::calcStateHashKey(states[stepsToGoInNextState]);

        visitDecisionNode(chosenOutcome);
    } else {
        ++chanceNodeVarIndex;
        visitChanceNode(chosenOutcome);
    }
    backupFunction->backupChanceNode(node, trialReward);
}

void THTS::visitDummyChanceNode(SearchNode* node) {
    State::calcStateFluentHashKeys(states[stepsToGoInNextState]);
    State::calcStateHashKey(states[stepsToGoInNextState]);

    if (node->children.empty()) {
        node->children.resize(1, nullptr);
        node->children[0] = createDecisionNode(1.0);
    }
    assert(node->children.size() == 1);

    visitDecisionNode(node->children[0]);
    backupFunction->backupChanceNode(node, trialReward);
}

int THTS::getUniquePolicy() {
    if (stepsToGoInCurrentState == 1) {
        uniquePolicyDueToLastAction = true;
        return getOptimalFinalActionIndex(states[1]);
    }

    std::vector<int> actionsToExpand =
        getApplicableActions(states[stepsToGoInCurrentState]);

    if (isARewardLock(states[stepsToGoInCurrentState])) {
        uniquePolicyDueToRewardLock = true;
        for (unsigned int i = 0; i < actionsToExpand.size(); ++i) {
            if (actionsToExpand[i] == i) {
                return i;
            }
        }
        assert(false);
    }

    std::vector<int> applicableActionIndices =
        getIndicesOfApplicableActions(states[stepsToGoInCurrentState]);
    assert(!applicableActionIndices.empty());

    if (applicableActionIndices.size() == 1) {
        uniquePolicyDueToPreconds = true;
        return applicableActionIndices[0];
    }

    // There is no clear, unique policy
    return -1;
}

SearchNode* THTS::createRootNode() {
    for (SearchNode* node : nodePool) {
        if (node) {
            if (!node->children.empty()) {
                std::vector<SearchNode*> tmp;
                node->children.swap(tmp);
            }
        } else {
            break;
        }
    }

    SearchNode* res = nodePool[0];

    if (res) {
        res->reset(1.0, stepsToGoInCurrentState);
    } else {
        res = new SearchNode(1.0, stepsToGoInCurrentState);
        nodePool[0] = res;
    }
    res->immediateReward = 0.0;

    lastUsedNodePoolIndex = 1;
    return res;
}

SearchNode* THTS::createDecisionNode(double const& prob) {
    assert(lastUsedNodePoolIndex < nodePool.size());

    SearchNode* res = nodePool[lastUsedNodePoolIndex];

    if (res) {
        res->reset(prob, stepsToGoInNextState);
    } else {
        res = new SearchNode(prob, stepsToGoInNextState);
        nodePool[lastUsedNodePoolIndex] = res;
    }
    calcReward(states[stepsToGoInCurrentState], appliedActionIndex,
               res->immediateReward);

    ++lastUsedNodePoolIndex;
    return res;
}

SearchNode* THTS::createChanceNode(double const& prob) {
    assert(lastUsedNodePoolIndex < nodePool.size());

    SearchNode* res = nodePool[lastUsedNodePoolIndex];

    if (res) {
        res->reset(prob, stepsToGoInCurrentState);
    } else {
        res = new SearchNode(prob, stepsToGoInCurrentState);
        nodePool[lastUsedNodePoolIndex] = res;
    }

    ++lastUsedNodePoolIndex;
    return res;
}

void THTS::setMaxSearchDepth(int _maxSearchDepth) {
    SearchEngine::setMaxSearchDepth(_maxSearchDepth);

    assert(initializer);
    initializer->setMaxSearchDepth(_maxSearchDepth);
}

void THTS::printConfig(std::string indent) const {
    SearchEngine::printConfig(indent);
    indent += "  ";

    switch(terminationMethod) {
        case TerminationMethod::TIME:
            Logger::logLine(indent + "Termination method: TIME",
                            Verbosity::VERBOSE);
            Logger::logLine(indent + "Timeout: " + std::to_string(timeout),
                            Verbosity::VERBOSE);
            break;
        case TerminationMethod::NUMBER_OF_TRIALS:
            Logger::logLine(indent + "Termination method: NUM TRIALS",
                            Verbosity::VERBOSE);
            Logger::logLine(
                indent + "Max num trials: " + std::to_string(maxNumberOfTrials),
                Verbosity::VERBOSE);
            break;
        case TerminationMethod::TIME_AND_NUMBER_OF_TRIALS:
            Logger::logLine(
                indent + "Termination method: TIME AND NUM TRIALS",
                Verbosity::VERBOSE);
            Logger::logLine(
                indent + "Timeout: " + std::to_string(timeout),
                Verbosity::VERBOSE);
            Logger::logLine(
                indent + "Max num trials: " + std::to_string(maxNumberOfTrials),
                Verbosity::VERBOSE);
            break;
    }
    Logger::logLine(
        indent + "Max num search nodes: " + std::to_string(maxNumberOfNodes),
        Verbosity::VERBOSE);
    Logger::logLine(
        indent + "Node pool size: " + std::to_string(nodePool.size()),
        Verbosity::VERBOSE);

    actionSelection->printConfig(indent);
    outcomeSelection->printConfig(indent);
    Logger::logLine(indent + "Trial length: CountDecisionNodes", Verbosity::VERBOSE);
    Logger::logLine(indent + "  Decision node count: " +
                    std::to_string(numberOfNewDecisionNodesPerTrial),
                    Verbosity::VERBOSE);
    initializer->printConfig(indent);
    backupFunction->printConfig(indent);
    recommendationFunction->printConfig(indent);
}

void THTS::printStepStatistics(std::string indent) const {
    if (uniquePolicyDueToLastAction) {
        Logger::logLine(
            indent + "Policy unique due to optimal last action",
            Verbosity::NORMAL);
    } else if (uniquePolicyDueToRewardLock) {
        Logger::logLine(
            indent + "Policy unique due to reward lock", Verbosity::NORMAL);
        Logger::logLine(
            indent + states[stepsToGoInCurrentState].toString(),
            Verbosity::VERBOSE);
    } else if (uniquePolicyDueToPreconds) {
        Logger::logLine(
            indent + "Policy unique due to single reasonable action",
            Verbosity::NORMAL);
    } else {
        Logger::logLine(
            indent + name + " step statistics:", Verbosity::NORMAL);
        indent += "  ";

        printStateValueCacheUsage(indent);
        printApplicableActionCacheUsage(indent);

        Logger::logLine(
            indent + "Performed trials: " + std::to_string(currentTrial),
            Verbosity::NORMAL);
        Logger::logLine(
            indent + "Created search nodes: " +
            std::to_string(lastUsedNodePoolIndex),
            Verbosity::NORMAL);
        Logger::logLine(
            indent + "Search time: " + std::to_string(lastSearchTime),
            Verbosity::NORMAL);
        Logger::logLine(
            indent + "Cache hits: " + std::to_string(cacheHits),
            Verbosity::VERBOSE);
        Logger::logLine(
            indent + "Max search depth: " + std::to_string(maxSearchDepth),
            Verbosity::VERBOSE);

        if (currentRootNode) {
            Logger::logLine(indent + "Q-value estimates:", Verbosity::VERBOSE);
            Logger::logLine(
                indent + "  Root node: " + getCurrentRootNode()->toString(),
                Verbosity::VERBOSE);

            for (size_t i = 0; i < currentRootNode->children.size(); ++i) {
                SearchNode const *child = currentRootNode->children[i];
                if (child) {
                    ActionState const &action = SearchEngine::actionStates[i];
                    Logger::logLine(
                        indent + "    " + action.toCompactString() + ": " +
                        child->toString(), Verbosity::VERBOSE);
                }
            }
        }

        Logger::logLine("", Verbosity::VERBOSE);
        actionSelection->printStepStatistics(indent);
        outcomeSelection->printStepStatistics(indent);
        initializer->printStepStatistics(indent);
        backupFunction->printStepStatistics(indent);
    }
}

void THTS::printRoundStatistics(std::string indent) const {
    Logger::logLine(indent + name + " round statistics:", Verbosity::NORMAL);
    indent += "  ";

    if (Logger::runVerbosity < Verbosity::VERBOSE) {
        printStateValueCacheUsage(indent, Verbosity::SILENT);
        printApplicableActionCacheUsage(indent, Verbosity::SILENT);
    }

    Logger::logLine(
        indent + "Number of remaining steps in first solved state: " +
        std::to_string(stepsToGoInFirstSolvedState),
        Verbosity::SILENT);
    Logger::logLine(
        indent + "Number of trials in initial state: " +
        std::to_string(numTrialsInInitialState),
        Verbosity::SILENT);
    Logger::logLine(
        indent + "Number of search nodes in initial state: " +
        std::to_string(numSearchNodesInInitialState),
        Verbosity::SILENT);

    Logger::logLine(
        indent + "Number of reward lock states: " +
        std::to_string(numRewardLockStates),
        Verbosity::NORMAL);
    Logger::logLine(
        indent + "Number of states with only one applicable action: " +
        std::to_string(numSingleApplicableActionStates),
        Verbosity::NORMAL);

    Logger::logLine("", Verbosity::VERBOSE);
    actionSelection->printRoundStatistics(indent);
    outcomeSelection->printRoundStatistics(indent);
    initializer->printRoundStatistics(indent);
    backupFunction->printRoundStatistics(indent);
}
