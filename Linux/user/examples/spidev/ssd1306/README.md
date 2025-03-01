![ssd1306](https://github.com/honechko/DS2450/raw/main/Linux/user/examples/spidev/ssd1306/ssd1306.png)  
```
# echo 0 spidev >/sys/devices/w1_bus_master1/20-594e480f4241/spi_bind
# echo 1 spidev >/sys/devices/w1_bus_master1/20-594e480f4241/spi_bind
# gcc spi_lsb.c -o spi_lsb
# ./spi_lsb /dev/spidev_20-594e480f4241.1
# echo -ne "\x8d\x14\xa7\xc8\xaf\x20\x01" >/dev/spidev_20-594e480f4241.0
# convert -resize 64x128^ -gravity center -crop 64x128+0+0 +repage -monochrome -dither FloydSteinberg mona_big.jpg mona1.pbm
# convert -resize 64x128^ -gravity center -crop 64x128+0+0 +repage -remap pattern:gray50 -dither FloydSteinberg mona_big.jpg mona.pbm
# convert -rotate 90 -resize 64x128^ -gravity center -crop 64x128+0+0 +repage -monochrome -dither FloydSteinberg landscape.jpg img.pbm
# tail -c $((64*128/8)) mona.pbm >/dev/spidev_20-594e480f4241.1
```  
![ssd1306](https://github.com/honechko/DS2450/raw/main/Linux/user/examples/spidev/ssd1306/ssd1306.jpg)  
