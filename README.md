# jpeg 解碼器

將 jpg 檔轉換為 bmp 檔

## 編譯

``` sh
mkdir build
cd build
cmake ..
make
```

## 執行

``` sh
./jpeg_decoder <jpeg file>
```
會在同一目錄得到一個 out.bmp

## 實做
idct實做請見 [這裡](https://hackmd.io/MYZhDYE4HZgRgLQCNoBNEBYBm4CmCBDLJfDXABgFYAOayjVSkYIA)

## 優化
目前效能瓶頸為進行霍夫曼編碼時採用C++ stl的map，可嘗試修改為自製之資料結構

此外idct也仍能進一步優化，可見 [這裡](http://blog.sina.com.cn/s/blog_4e19c4c80100gjbf.html)
