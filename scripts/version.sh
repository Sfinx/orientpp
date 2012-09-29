#!/bin/sh
# get current changeset from git
Changeset=`git log --pretty=format:\"%h\" -1`
echo -e "\n#include \"orient.h\"\n\n// app version\n\nconst char *OrientPP::ORIENTPP_CHANGESET = $Changeset;" > version.cpp
ChangesetNumber=`git log --oneline | wc -l`
echo "int OrientPP::ORIENTPP_CHANGESET_NUMBER = $ChangesetNumber;" >> version.cpp
# create new build number
BuildNumber=0
echo "int OrientPP::ORIENTPP_BUILD_NUMBER = $BuildNumber;" >> version.cpp
echo OrientPP: changeset $Changeset, changeset number $ChangesetNumber, build number 0
