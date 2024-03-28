FROM node:19.6.0

# 后续需要 curator 清理 ElasticSearch 日志

WORKDIR /wework
COPY binding.gyp ./
COPY ./lib ./lib/

# 安装 Python 3
RUN apt-get update && \
    apt-get install -y --no-install-recommends python3 && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# 设置 Python 3 为默认的 python 版本
RUN ln -s /usr/bin/python3 /usr/bin/python


RUN touch index.js
# 设置保活功能，可以根据实际需要调整命令
CMD ["node", "-e", "setInterval(function() { console.log('Container is alive'); }, 60000);"]
