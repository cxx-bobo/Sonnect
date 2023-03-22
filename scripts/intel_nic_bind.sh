export dpdk_root=/usr/src/dpdk-21.11/
export interface_name=ens192

# Setup non-iommu mode (execute this inside virtual machine)
echo 1 > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode

# Bind interface to vfio driver
cd $dpdk_root/usertools
./dpdk-devbind.py -b vfio-pci $interface_name --force
./dpdk-devbind.py -s