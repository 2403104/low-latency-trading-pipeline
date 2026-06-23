#!/bin/bash

echo "Initializing Sandbox Hugepages Setup (256 x 2MB = 512MB)"

echo 256 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages > /dev/null

if mount | grep -q "hugetlbfs"; then
  echo "[INFO] hugetlbfs filesystem layer is already active."
else 
  echo "[INFO] hugetlbfs not detected. Mounting filesystem layer..."
  sudo mkdir -p /dev/hugepages
  sudo mount -t hugetlbfs nodev /dev/hugepages
fi

TOTAL_PAGES=$(cat /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages)
FREE_PAGES=$(cat /sys/kernel/mm/hugepages/hugepages-2048kB/free_hugepages)

echo "--------------------------------------------------------"
echo "Target Configuration Complete:"
echo "  - Total Hugepages Allocated : $TOTAL_PAGES"
echo "  - Free Hugepages Available  : $FREE_PAGES"
echo "  - Pinned Low-Latency Memory  : $(($TOTAL_PAGES * 2)) MB"
echo "--------------------------------------------------------"

