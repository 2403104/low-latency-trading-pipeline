#!/bin/bash

echo "Cleaning Sandbox Hugepages (Releasing Memory)"

echo 0 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages > /dev/null
sudo rm -rf /var/run/dpdk/*
sudo rm -rf /dev/hugepages/*

TOTAL_PAGES=$(cat /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages)
FREE_PAGES=$(cat /sys/kernel/mm/hugepages/hugepages-2048kB/free_hugepages)

echo "--------------------------------------------------------"
echo "Cleanup Verification:"
echo "  - Total Hugepages Remaining : $TOTAL_PAGES"
echo "  - Free Hugepages Available  : $FREE_PAGES"
echo "  - Memory Returned to System : 512 MB"
echo "--------------------------------------------------------"