# Setup hugepage
echo 8192 > /sys/devices/system/node/node0/hugepages/hugepages-2048kB/nr_hugepages
echo 8192 > /sys/devices/system/node/node1/hugepages/hugepages-2048kB/nr_hugepages
# echo 8192 > /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages
# echo 8192 > /sys/devices/system/node/node1/hugepages/hugepages-1048576kB/nr_hugepages
mkdir /mnt/huge
mount -t hugetlbfs pagesize=2MB /mnt/huge
# mount -t hugetlbfs pagesize=1GB /mnt/huge