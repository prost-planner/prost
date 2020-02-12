from collections import defaultdict


class IPCScores(object):
    """Compute the IPC quality score.

    The IPC score is computed over the list of runs for each task. Since
    filters only work on individual runs, we can't compute the score with a
    single filter, but it is possible by using two filters: *store_rewards* saves
    the list of rewards per task in a dictionary whereas *add_score* uses the
    stored rewards to compute IPC quality scores and adds them to the runs.

    """

    def __init__(self):
        # We are using normal dict on purpose.
        self.tasks_to_rewards = defaultdict(list)

    def _get_task(self, run):
        return (run["domain"], run["problem"])

    def _compute_score(self, reward, min_reward, max_reward, all_rewards):
        if reward is None or reward <= min_reward:
            # If the planner reward is worse than the minimum,
            # or it has failed to compute the average reward,
            # it should get a score of 0.
            return 0.0
        assert all_rewards
        if max_reward == None:
            max_reward = max(all_rewards)
        return (reward - min_reward) / (max_reward - min_reward)

    def store_rewards(self, run):
        score = run.get("average_reward")
        task = self._get_task(run)

        if score is not None:
            self.tasks_to_rewards[task].append(score)
        return True

    def add_score(self, run):
        run["ipc_score"] = self._compute_score(
            run.get("average_reward"),
            run.get(
                "min_score"
            ),  # We should change the names in the properties files to use 'reward'
            run.get("max_score"),
            self.tasks_to_rewards[self._get_task(run)],
        )
        return run
