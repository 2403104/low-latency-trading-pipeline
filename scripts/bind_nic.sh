echo "--------------------------------------------------------"
echo "Binding [NIC] with [vfio-cpi]."

sudo modprobe vfio-pci

sudo dpdk-devbind.py --bind=vfio-pci 0000:04:00.0

echo "--- Status Check ---"

sudo dpdk-devbind.py --status | grep 04:00.0

echo "Binding Done."
echo "--------------------------------------------------------"