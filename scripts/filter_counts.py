#!/usr/bin/env python

# we're using python 3.x style print but want it to work in python 2.x,
from __future__ import print_function
import re, os, argparse, sys, math, warnings, subprocess, threading, shutil
import tempfile
import platform


# make sure scripts/internal is on the pythonpath.
sys.path = [ os.path.abspath(os.path.dirname(sys.argv[0])) + "/internal" ] + sys.path

# for ExitProgram and RunCommand
from pocolm_common import *


parser = argparse.ArgumentParser(description="Usage: "
                                 "filter_counts.py [options] <all-count-dir> <ngram-order> <filter-count-dir>"
                                 "e.g.:  filter_counts.py data/counts_3 3 data/counts_f3"
                                 "This script filters int counts of training data by dev data.");

parser.add_argument("--parallel", type=str, default='true',
                    choices=['true','false'],
                    help="If true, process multiple data-sources in parallel.")
parser.add_argument("--verbose", type=str, default='false',
                    choices=['true','false'],
                    help="If true, print commands as we execute them.")
parser.add_argument("all_count_dir",
                    help="Specify <all_count_dir> the data-source")
parser.add_argument("ngram_order", type=int,
                    help="Specify the order of ngram")
parser.add_argument("filter_count_dir",
                    help="Specify <filter_count_dir> the destination to puts the filter counts")

args = parser.parse_args()


# this reads the 'names' file (which has lines like "1 switchboard", "2 fisher"
# and so on), and returns a dictionary from integer id to name.
def ReadNames(names_file):
    try:
        f = open(names_file, "r");
    except:
        sys.exit("filter_counts.py: failed to open --names={0}"
                 " for reading".format(names_file))
    number_to_name = { }
    for line in f:
        try:
            [ number, name ] = line.split();
            number = int(number)
        except:
            sys.exit("filter_counts.py: Bad line '{0}' in names file {1}".format(
                    line[0:-1], names_file))
        if number in number_to_name:
            sys.exit("filter_counts.py: duplicate number {0} in names file {1}".format(
                    number, names_file))
        number_to_name[number] = name
    f.close()
    return number_to_name

def GetNumTrainSets(all_count_dir):
    f = open(all_count_dir + '/num_train_sets')
    # the following should not fail, since we validated all_count_dir.
    num_train_sets = int(f.readline())
    assert f.readline() == ''
    f.close()
    return num_train_sets

# copy over some meta-info into the 'counts' directory.
def CopyMetaInfo(all_count_dir, filter_count_dir):
    for f in ['num_train_sets', 'num_words', 'names', 'words.txt' ]:
        try:
            src = all_count_dir + os.path.sep + f
            dest = filter_count_dir + os.path.sep + f
            shutil.copy(src, dest)
        except:
            ExitProgram('error copying {0} to {1}'.format(src, dest))
    f = 'unigram_weights'
    src = all_count_dir + os.path.sep + f
    if os.path.exists(src):
        try:
            dest = filter_count_dir + os.path.sep + f
            shutil.copy(src, dest)
        except:
            ExitProgram('error copying {0} to {1}'.format(src, dest))

def IsCygwin():
    return platform.system()[0:3].lower() == 'win' or platform.system()[0:3].lower() == 'cyg'


# save the n-gram order.
def SaveNgramOrder(filter_count_dir, ngram_order):
    try:
        f = open('{0}/ngram_order'.format(filter_count_dir), 'w')
    except:
        ExitProgram('error opening file {0}/ngram_order for writing'.format(filter_count_dir))
    assert ngram_order >= 2
    print(ngram_order, file=f)
    f.close()


# this function filters the counts.

# if num_splits == 0, then it dumps
# its output to {filter_count_dir}/int.{n}.{o} with n = 1..num_train_sets.
# (note: n,o is supplied to this function).
#
# If num-splits >= 1, then it dumps its output to
# {filter_count_dir}/int.{n}.split{j} with n = 1..num_train_sets, j=1..num_splits.

def FilterCountsSingleProcess(all_count_dir, filter_count_dir, o, n, num_splits = 0):
    if num_splits == 0:
        filter_counts_output = '> {0}/int.{1}.{2}'.format(filter_count_dir, n, o)

    else:
        assert num_splits >= 1
        filter_counts_output = '/dev/stdout | split-int-counts ' + \
            ' '.join([ "{0}/int.{1}.split{2}".format(filter_count_dir, n, j)
                       for j in range(1, num_splits + 1) ])

    command = "bash -c 'set -o pipefail; extract-latest-histories "\
              "<{all_count_dir}/int.dev.{o} >{filter_count_dir}/lastest_hist.dev.{o}'".format(all_count_dir = all_count_dir,
                                              n = n , o = o,
                                              filter_count_dir = filter_count_dir)
    log_file = "{filter_count_dir}/log/extract-histories.{n}.{o}.log".format(
        filter_count_dir = filter_count_dir, n = n, o = o)
    RunCommand(command, log_file, args.verbose == 'true')

    if o < args.ngram_order:
        # merge lastest_hist with order + 1
        command = "bash -c 'set -o pipefail; export LC_ALL=C; "\
                  "cat {filter_count_dir}/lastest_hist.dev.{o} "\
                  "    {filter_count_dir}/lastest_hist.{p} | "\
                  " sort -u > {filter_count_dir}/lastest_hist.{o}'".format(all_count_dir = all_count_dir,
                                                  n = n , o = o, p = o + 1,
                                                  filter_count_dir = filter_count_dir)
        log_file = "{filter_count_dir}/log/merge-histories.{n}.{o}.log".format(
            filter_count_dir = filter_count_dir, n = n, o = o)
        RunCommand(command, log_file, args.verbose == 'true')
    else:
        try:
            src = filter_count_dir + os.path.sep + 'lastest_hist.dev.' + o
            dest = filter_count_dir + os.path.sep + 'lastest_hist.' + o
            shutil.copy(src, dest)
        except:
            ExitProgram('error copying {0} to {1}'.format(src, dest))


    command = "bash -c 'set -o pipefail; filter-int-counts {all_count_dir}/int.{n}.{o} "\
              "{filter_count_dir}/lastest_hist.{o} {filter_counts_output}'".format(all_count_dir = all_count_dir,
                                              filter_count_dir = filter_count_dir,
                                              n = n , o = o,
                                              filter_counts_output = filter_counts_output)
    log_file = "{filter_count_dir}/log/filter_counts.{n}.{o}.log".format(
        filter_count_dir = filter_count_dir, n = n, o = o)
    RunCommand(command, log_file, args.verbose == 'true')


# that contains all the dev-data's counts; this will be used in likelihood
# evaluation.
def CopyDevData(all_count_dir, filter_count_dir):
    for f in ['int.dev'] +  [ "int.dev.{0}".format(o) for o in range(2, args.ngram_order + 1) ]:
      try:
          src = all_count_dir + os.path.sep + f
          dest = filter_count_dir + os.path.sep + f
          shutil.copy(src, dest)
      except:
          ExitProgram('error copying {0} to {1}'.format(src, dest))


# make sure 'scripts' and 'src' directory are on the path
os.environ['PATH'] = (os.environ['PATH'] + os.pathsep +
                      os.path.abspath(os.path.dirname(sys.argv[0])) + os.pathsep +
                      os.path.abspath(os.path.dirname(sys.argv[0])) + "/../src")



if os.system("validate_count_dir.py " + args.all_count_dir) != 0:
    ExitProgram("command validate_count_dir.py {0} failed".format(args.all_count_dir))

if args.ngram_order < 2:
    ExitProgram("ngram-order is {0}; it must be at least 2.  If you "
                "want a unigram LM, do it by hand".format(args.ngram_order))

# read the variable 'num_train_sets'
# from the corresponding file in all_count_dir  This shouldn't fail
# because we just called validate_int-dir.py..
f = open(args.all_count_dir + "/num_train_sets")
num_train_sets = int(f.readline())
f.close()

if not os.path.isdir(args.filter_count_dir):
    try:
        os.makedirs(args.filter_count_dir+'/log')
    except:
        ExitProgram("error creating directory " + args.filter_count_dir)


CopyMetaInfo(args.all_count_dir, args.filter_count_dir)

SaveNgramOrder(args.filter_count_dir, args.ngram_order)

print("filter_counts.py: filtering counts", file=sys.stderr)
threads = []
for n in range(1, num_train_sets + 1):
  for o in range(args.ngram_order, 1, -1):
    threads.append(threading.Thread(target = FilterCountsSingleProcess,
                                    args = [args.all_count_dir, args.filter_count_dir,
                                            str(o), str(n)] ))
    threads[-1].start()
    if args.parallel == 'false':
        threads[-1].join()

if args.parallel == 'true':
    for t in threads:
        t.join()

CopyDevData(args.all_count_dir, args.filter_count_dir)
print("filter_counts.py: done", file=sys.stderr)

if os.system("validate_count_dir.py " + args.filter_count_dir) != 0:
    ExitProgram("command validate_count_dir.py {0} failed".format(args.filter_count_dir))

