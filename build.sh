#!/bin/sh
A=/home/ken/workspace/exynos4412/exynos4412_Kernel_3.0
B=/home/ken/workspace/exynos4412/tmp
echo 'Copying new files......Please wait'

find . -mtime -1 -regex ".*\.c\|.*\.h" > line
for filename in `cat line`
do
#if [ ! -f "$B"/"$filename" ]; then
Val1=$(md5sum "$filename"|cut -d ' ' -f1)
TempName=${filename#.*/}
Val2=$(md5sum "$B"/"$TempName"|cut -d ' ' -f1)
if [ $Val1 != $Val2 ]; then
cp -rf "$filename" "$B"/"$TempName"
let Count++
echo "$filename" is diffrent,Copyed...
fi
#fi
done
rm -rf "$A"/line

echo "Copy files done... $Count file(s) are copyed."
echo 'Entry into the compile directories...'
cd  ../tmp
echo 'Building kernel image'
make zImage
echo 'Build successful...'
echo 'Copying zImage to oe-build...'
cp arch/arm/boot/zImage ../exynos4412_Kernel_3.0/oe-build/
echo 'All Done...'
