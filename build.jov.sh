set -e

cd ~/Code/JovialTiles/build/
make 
cd ..
cp build/jovial_tiles .
./jovial_tiles ./assets/test.png -size 16 16 -blob
