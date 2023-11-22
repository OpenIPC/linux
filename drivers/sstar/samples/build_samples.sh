rm -rf output
mkdir output
filename=$(ls *.c)
for file in ${filename};do
	$CROSS_COMPILE\gcc ${file} -o output/${file%.*}
	$CROSS_COMPILE\strip --strip-unneeded output/${file%.*}
done

cd output
mv -f riu riu_r
ln -s riu_r riu_w
ln -s riu_r riux32_w
ln -s riu_r riux32_r
