#!/bin/sh
A=/home/ken/workspace/exynos4412/Graduation_Prj
B=/home/ken/workspace/exynos4412/tmp
Count=0
let Count++
let Count--
echo 'Copying new files......Please wait'

#find "$A" -amin -10 -regex ".*\.c\|.*\.h" > line
find "$A" -cmin -150 -type f > line
for filename in `cat line`
do
TempName=${filename:46}
if [ -e "$B"/"$TempName" ]; then
Val1=$(md5sum "$filename"|cut -d ' ' -f1)
Val2=$(md5sum "$B"/"$TempName"|cut -d ' ' -f1)
if [ $Val1 != $Val2 ]; then
cp -rf "$filename" "$B"/"$TempName"
let Count++
echo "$filename" is diffrent,Copyed...
fi
else
echo "$filename" is not exist...Copyed
cp -rf "$filename" "$B"/"$TempName"
let Count++
fi
done

rm -rf line
echo "Copy files done... $Count file(s) are copyed."
echo 'Entry into the compile directories...'
cd  ../tmp
echo 'Building kernel image'
make zImage
echo 'Build successful...'
echo 'Copying zImage to oe-build...'
cp arch/arm/boot/zImage "$A"/oe-build/
echo 'All Done...'
