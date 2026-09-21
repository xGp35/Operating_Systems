#! /usr/bin/env python3

import sys
import os
import subprocess
import re
import argparse

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PAGING_SCRIPT = os.path.join(SCRIPT_DIR, 'paging-policy.py')

ALL_POLICIES = ['OPT', 'RAND', 'FIFO', 'LRU', 'MRU', 'CLOCK']

def parse_args():
    parser = argparse.ArgumentParser(
        description='Compare cache replacement policies using paging-policy.py')
    parser.add_argument('-a', '--addresses', default='-1',
                        help='comma-separated pages to access; -1 means randomly generate')
    parser.add_argument('-A', '--tracefile', default='',
                        help='file containing a comma-separated trace (read as -a)')
    parser.add_argument('-f', '--addressfile', default='',
                        help='a file with one address per line (passed to paging-policy.py -f)')
    parser.add_argument('-n', '--numaddrs', default='10',
                        help='number of random addresses to generate (when -a is -1)')
    parser.add_argument('-C', '--cachesize', required=True,
                        help='size of the page cache, in pages')
    parser.add_argument('-m', '--maxpage', default='10',
                        help='max page number for random generation')
    parser.add_argument('-s', '--seed', default='0',
                        help='random number seed')
    parser.add_argument('-b', '--clockbits', default='2',
                        help='clock bits for CLOCK policy')
    parser.add_argument('--policies', default=','.join(ALL_POLICIES),
                        help='comma-separated list of policies to compare (default: all)')
    return parser.parse_args()

def run_policy(policy, args):
    cmd = [sys.executable, PAGING_SCRIPT,
           '-a', args.addresses,
           '-C', args.cachesize,
           '-p', policy,
           '-s', args.seed,
           '-n', args.numaddrs,
           '-m', args.maxpage,
           '-b', args.clockbits,
           '-c', '-N']
    if args.addressfile:
        cmd += ['-f', args.addressfile]

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        return None

    match = re.search(r'FINALSTATS hits (\d+)\s+misses (\d+)\s+hitrate ([\d.]+)', result.stdout)
    if not match:
        return None
    return int(match.group(1)), int(match.group(2)), float(match.group(3))

def main():
    if not os.path.exists(PAGING_SCRIPT):
        print('Error: paging-policy.py not found at %s' % PAGING_SCRIPT)
        sys.exit(1)

    args = parse_args()

    if args.tracefile:
        with open(args.tracefile) as f:
            args.addresses = f.read().strip().split('\n')[0].strip()

    policies = [p.strip().upper() for p in args.policies.split(',')]
    max_name = max(len(p) for p in policies)

    for policy in policies:
        stats = run_policy(policy, args)
        if stats is None:
            print('%-*s - ERROR: could not get results' % (max_name, policy))
        else:
            hits, misses, hitrate = stats
            print('%-*s - hits %d   misses %d   hitrate %.2f' %
                  (max_name, policy, hits, misses, hitrate))

if __name__ == '__main__':
    main()
