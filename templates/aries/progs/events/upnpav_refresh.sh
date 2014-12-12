#!/bin/sh
xmldbc -s /runtime/scan_media/total_file "0"
xmldbc -s /runtime/scan_media/total_scan_file "0"
xmldbc -s /runtime/scan_media/scan_done "0"

echo "Start scan media !" > /dev/console
ScanMedia &
