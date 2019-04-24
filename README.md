# JPEG 解碼器

將 jpg 檔轉換為 bmp 檔

僅支援 baseline jpg

## 故事

請**就讀臺灣大學的同學**，看過這篇文章[吳家麟，我教育你](https://city-of-infinity.com/app/b/%E5%85%AB%E5%8D%A6/a/%E5%90%B3%E5%AE%B6%E9%BA%9F%EF%BC%8C%E6%88%91%E6%95%99%E8%82%B2%E4%BD%A0?id=5cbcd478271ae627b77544ff) （[備份](https://hackmd.io/ctyDPW8dTIuHCaCDkgw-SQ?both)）之後，再繼續閱讀，感謝！

## 關於 JPEG

我用 rust 重新實作了一個 JPEG 解碼器，並且撰寫了一份 JPEG 教學：[跟我寫 JPEG 解碼器](https://github.com/MROS/jpeg_tutorial)，我在綜合多份資料之後，加上自己的想法，並且爲其繪製多張圖片，力求做到清晰易懂，建議想要學習 JPEG 的人閱讀這份文件，若不熟悉 rust 語言，可以再參考以 C++ 撰寫的本專案 。

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
idct 實做請見 [這裡](https://hackmd.io/MYZhDYE4HZgRgLQCNoBNEBYBm4CmCBDLJfDXABgFYAOayjVSkYIA)

## 優化
目前效能瓶頸為進行霍夫曼編碼時採用 C++ stl 的 map，可嘗試修改為自製之資料結構

此外 idct 也仍能進一步優化，可見 [這裡](https://dsp.stackexchange.com/questions/51311/computation-of-the-inverse-dct-idct-using-dct-or-ifft)

