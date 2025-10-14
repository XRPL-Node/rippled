#!/bin/bash

if [[ $# -ne 3 || "$1" == "--help" || "$1" = "-h" ]]
then
  name=$( basename $0 )
  cat <<- USAGE
  Usage: $name pr "title" "description"

  * All commits in the specified PR will be squashed and a new commit prepared
    with the provided title and description as commit message.
  * This script will not push the new commit. You will need to do so yourself
    by force-pushing, since you will be rewriting history. You must be the
    author of the PR or a maintainer of the repository in order to perform this
    operation.
  * The 'gh' CLI tool must be installed and authenticated.
  * To write a multiline description, you can use "\$(cat <<EOF
line 1
line 2
EOF
)" to pass it as a single argument.
  * If you get a '[rejected]' error when updating the target branch and then
    locally merging the changes into the source branch, it is likely because the
    source branch already exists on your machine (e.g. you ran this script
    multiple times). In that case, you can delete the local source branch
    (e.g. 'git branch -D [source]') and try again.

USAGE
exit 0
fi

pr="$1"
shift

title=$1
shift

description=$1
shift

set -e

echo "Checking workspace."
diff=$(git status --porcelain)
if [ -n "${diff}" ]; then
  echo "Error: Workspace is not clean. Please commit or stash your changes."
  exit 1
fi

echo "Checking out PR ${pr}."
gh pr checkout "${pr}"

echo "Getting the target branch of the PR."
target=$(gh pr view --json "baseRefName" --jq '.baseRefName')
if [ -z "${target}" ]; then
  echo "Error: Could not determine target branch of PR ${pr}."
  exit 1
fi

echo "Getting the source branch of the PR."
source=$(git branch --show-current)

echo "Ensuring the PR source branch '${source}' is up to date with the target branch '${target}'."
git checkout ${target}
git pull --rebase
gh pr checkout "${pr}"
git merge ${target} --no-edit

# TODO: check for conflicts and abort if there are any.

echo "Squashing commits in the PR."
git reset --soft $(git merge-base ${target} HEAD)
git commit -S -m "${title}" -m "${description}"

# We assume that external contributors will create a fork in their personal
# repository, i.e. the repo owner matches the currently logged in user. In that
# case they can push directly to their branch. If the owner is 'XRPLF', we also
# push directly to the branch, as we assume that the user running this script
# will be a maintainer.
echo "Gathering user details."
owner=$(gh pr view --json "headRepositoryOwner" --jq '.headRepositoryOwner.login')
user=$(gh api user --jq '.login')
echo "The PR is owned by '${owner}'. The current user is '${user}'."
if [ "${owner}" = 'XRPLF' ] || [ "${owner}" = "${user}" ]; then
  remote="origin"
else
  remote="${owner}"
fi

if [ "${remote}" = "origin" ]; then
  cat << EOF
----------------------------------------------------------------------
This script will not push. Verify everything is correct, then force
push to the source branch using the following command:

git push --force-with-lease origin ${source}

Remember to navigate back to your previous branch after pushing.
----------------------------------------------------------------------
EOF
else
  cat << EOF
----------------------------------------------------------------------
This script will not push. Verify everything is correct, then force
push to the fork using the following command:

git remote add ${remote} git@github.com:${remote}/rippled.git
git fetch ${remote}
git push --force-with-lease ${remote} ${source}
git remote remove ${remote}

Remember to navigate back to your previous branch after pushing.
----------------------------------------------------------------------
EOF
fi
