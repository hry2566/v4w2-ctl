# v4w2-ctl

 * v4w2は、Video for Windows 2の略です。  
 * v4l2(Video for Linux 2)-ctlの機能が欲しくてWindows用に作成しました。  
 　（DirectShow等の知識はゼロなのでネット上サンプルのほぼコピーです。）  
 * 出来るだけv4l2-ctlに似せてあります。  
 
## 開発環境  
 * Windows11 Home  
 * MinGW + VSCode (Windows SDK)
  
## スクリーンショット  
![image](https://user-images.githubusercontent.com/86605611/211980718-02feac41-9603-49c3-a4fd-badbdbda3102.png)  
![image](https://user-images.githubusercontent.com/86605611/212462382-ce0d7689-e9dc-4f12-8389-89a09f920fcf.png)
  
## 実装
 ・デバイス一覧  
 ・デバイスのフォーマット/フレームサイズ/フレームレートの一覧  
 ・デバイスの対応している設定項目一覧
  
## 対応フォーマット  
・format_types.txtに無いフォーマットは「1196444237」等の数値が表示されます。type_list.txtから4文字(MJPG等)をformat_types.txtへ追記  
