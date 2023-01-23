<h1 align="center">
  ⭕ Linux Shell ⭕
</h1>

<h4 align="center">A Shell implementation for Linux 🙂 <br>
By far not a complete one, it implements some very basic features.
</h4>
<br>
<p align="center">
  <a href="#-features">Features</a>&nbsp;&nbsp;&nbsp;|&nbsp;&nbsp;&nbsp;
  <a href="#-flow">Flow</a>
</p>

## 📍 Features
- [x] Unit commands with multiple parameters: 
  - ls -la
  <br>
- [x] Chained commands using "|":
  - ls -la | grep "foo"
  - ls -la | grep "foo" | more
  <br>
- [x] Redirect terminal input and output to external files using "<" for default input and ">" for default output:
  - sort < foo.txt > bar.txt
  <br>
- [x] Redirect terminal output to external files using ">>". Unlike the ">" operator, if the file already exists, it will append the output to the file content:
  - sort < foo.txt >> bar.txt
  <br>
- [x] Mixed operators:
  - sort < foo.txt | grep "x" > bar.txt
  - sort < foo.txt | grep "x" >> bar.txt

## 📍 Flow
![image](https://user-images.githubusercontent.com/18491745/213947652-74e693ca-e89c-4d2a-87be-f8f20846f1ec.png)
