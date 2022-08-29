#!/usr/bin/python

# Copyright (c) 2016-2022 Demiurge Studios Inc. All rights reserved.
#
# Generate a directory structure that
# can be used as a local maven repo
# for Gradle.
#
# Originally from: https://discuss.gradle.org/t/need-a-gradle-task-to-copy-all-dependencies-to-a-local-maven-repo/13397/10


import sys
import os
import subprocess
import glob
import shutil

# Windows prefix and separator.
if os.name == 'nt':
    prefix = u'\\\\?\\'
    sep = '\\'
else:
    prefix = ''
    sep = '/'


def main(argv):
    project_dir = os.path.normpath(os.path.dirname(os.path.realpath(__file__)))
    repo_dir = os.path.normpath(os.path.join(project_dir, "local-maven-repo"))
    input_dir = os.path.normpath(os.path.join(project_dir, "../../../Intermediate/Gradle/Debug"))
    if os.path.isdir(prefix + repo_dir):
        shutil.rmtree(prefix + repo_dir)

    cache_files = os.path.join(input_dir, "caches" + sep + "modules-*" + sep + "files-*")
    for cache_dir in glob.glob(cache_files):
        for cache_group_id in os.listdir(prefix + cache_dir):
            cache_group_dir = os.path.join(cache_dir, cache_group_id)
            repo_group_dir = os.path.join(repo_dir, cache_group_id.replace('.', sep))
            for cache_artifact_id in os.listdir(prefix + cache_group_dir):
                cache_artifact_dir = os.path.join(cache_group_dir, cache_artifact_id)
                repo_artifact_dir = os.path.join(repo_group_dir, cache_artifact_id)
                for cache_version_id in os.listdir(prefix + cache_artifact_dir):
                    cache_version_dir = os.path.join(cache_artifact_dir, cache_version_id)
                    repo_version_dir = os.path.join(repo_artifact_dir, cache_version_id)
                    if not os.path.isdir(prefix + repo_version_dir):
                        os.makedirs(prefix + repo_version_dir)
                    cache_items = os.path.join(cache_version_dir, "*" + sep + "*")
                    for cache_item in glob.glob(cache_items):
                        cache_item_name = os.path.basename(cache_item)
                        repo_item_path = os.path.join(repo_version_dir, cache_item_name)
                        print "%s:%s:%s (%s)" % (cache_group_id, cache_artifact_id, cache_version_id, cache_item_name)
                        shutil.copyfile(prefix + cache_item, prefix + repo_item_path)
    return 0

if __name__ == "__main__":
    sys.exit(main(sys.argv))
