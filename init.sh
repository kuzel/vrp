#!/bin/sh

module="vrp6"
device="burst"
mode=444

rm -f /dev/${device}
/sbin/rmmod ./$module.ko

/sbin/insmod ./$module.ko $* || exit 1

#major=$(cat /sys/module/${module}/parameters/burst_major)
major=$(awk "\$2==\"$device\" {print \$1}" /proc/devices)
echo major ${major}

mknod /dev/${device} c $major 0
group="staff"

chgrp $group /dev/${device}
chmod $mode /dev/${device}
