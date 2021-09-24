#!/usr/bin/python3

# Script to import kegs tars into git in order, simulating
# some sort of history


import os,sys
from stat import *
from datetime import date

if len(sys.argv)>2:
    # We're importing one or more tar files from the command line
    # Usage: create_repo.py <repo directory> <gzipped archives>+
    repo=sys.argv[1]
    versions=sys.argv[2:]
else:
    # no files specified, so we're starting from scratch and
    # importing all the gz files from the current directory
    repo = "tmp-kegs"
    versions = os.popen("ls -1tr | grep gz").read().split()

    # Danger ... this is dangerous!

    os.system("rm -rf "+repo)

    # Create a git repository and initialize it:
    os.system("git init "+repo)

# All of these are authored by Kent, so, we'll set things up as though
# we are him, at least locally.

username="Kent Dickey"
email="kegs@someplace.online" # Kent prefers to stay anonymous from robo spammers

os.system("cd "+repo+"; git config user.name '"+username+"'; git config user.email '"+email+"';")

for version in versions:
    # This is clearly weird, but we we want git to notice removed,
    # changed, and moved files, so ... we're going to delete everything
    # and let git 'find' things every time:

    # once again, the rm -rf is terrifyingly dangerous:
    os.system("cd "+repo+"; rm -rf *")

    # Tell nervous observers (that'd be me) what's going on:
    print("Uncompressing %s" % version)

    # Cat the tar file, and uncompress it in the repo directory
    # The tar files all have the version in them, so we will strip
    # the first directory with  --strip-components=
    os.system("cat %s | (cd %s; tar  --strip-components=1 -xzf -)" % (version, repo))

    # The tar files preserved their dates, so... let's use that date as the
    # commit date!

    stamp = os.stat(version).st_mtime
    creation = date.fromtimestamp(stamp).strftime("%c")

    print ("Creation: ", creation)
    # tell git to add everything here.
    os.system("cd "+repo+"; git add -u . && git add *")

    # Tell git to commit the changes:
    os.system("cd "+repo+"; GIT_COMMITTER_DATE='%s' git commit --date='%s' -m 'KEGS from %s'" % (creation, creation, version))

    # Lets create a tag as well, since this shipped in a tar file:
    tag = '.'.join(version.split('.')[:-2])
    os.system("cd "+repo+"; GIT_COMMITTER_DATE='%s' git tag -a %s -m 'KEGS from %s'" % (creation, tag, version))


 
