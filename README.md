# v4w2-ctl

 * v4w2は、Video for Windows 2の略です。  
 * v4l2(Video for Linux 2)-ctlの機能が欲しくてWindows用に作成しました。  
 　（DirectShow等の知識はゼロなのでネット上サンプルのほぼコピーです。）  
 * 出来るだけv4l2-ctlに似せてあります。  
 
## 開発環境  
 * Windows11 Home  
 * MinGW + VSCode (Windows SDK)
  
## スクリーンショット  
![image](https://user-images.githubusercontent.com/86605611/211966086-d4d7a4b7-6f87-4f24-b3ae-98f622ee15f5.png)  
  
## 実装
 ・デバイス一覧  
 ・デバイスのフォーマット/フレームサイズ/フレームレートの一覧  
  
## 対応フォーマット  
・format_types.txtに無いフォーマットは「1196444237」等の数値が表示されます。type_list.txtから4文字(MJPG等)をformat_types.txtへ追記  
