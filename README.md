# 企业微信会话内容存档 node.js 版

环境：linux

1. 实现思路：
   .cpp(用来调用 WeWorkFinanceSdk.h 里的方法)  ->  编译成 .node  ->  js引入 .node 就可以调用相关函数了

2. 工具
   安装 node-gyp :
   npm install -g node-gyp

3. 编译 （需要在 linux 环境下编译，可以用本地 docker）
   编译 .cpp 成 .node 的命令: node-gyp configure build


使用方法： 直接把 lib 考到项目里就可以用，用法参考 example.js
