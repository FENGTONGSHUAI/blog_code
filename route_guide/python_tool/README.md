# 工具目的
使用python实现简单的pb消息编解码, 便于验证信息

# python安装
这里使用的protoc版本是31.0, 需要的python版本比较高, 要大于3.9, 我这里安装的是3.11, 使用源码安装, 流程如下:
```shell
# 1. 安装编译依赖
sudo yum groupinstall "Development Tools" -y
sudo yum install openssl-devel bzip2-devel libffi-devel zlib-devel readline-devel sqlite-devel wget -y

# 2. 下载Python源码
wget https://www.python.org/ftp/python/3.11.9/Python-3.11.9.tar.xz
tar -xJf Python-3.11.9.tar.xz
cd Python-3.11.9

# 3. 编译安装
./configure --enable-optimizations
make -j$(nproc)
sudo make altinstall  # 使用altinstall避免覆盖系统Python

# 4. 验证
python3.11 --version

# 5. 创建软链接
sudo ln -sf /usr/local/bin/python3.11 /usr/bin/python311
sudo ln -sf /usr/local/bin/pip3.11 /usr/bin/pip311
```

安装完成之后, 执行:
```shell
sudo pip311 install protobuf
```

# 代码运行
首先执行generate_python_pb.sh生成python pb文件
```shell
sh generate_python_pb.sh
```
然后执行python脚本, 相应的, 脚本执行也要用python3.11
```shell
 python311 parse_pb.py 
```