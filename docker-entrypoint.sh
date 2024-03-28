#!/bin/sh

env >> /etc/default/locale

# 需要输出系统日志调试时启用
# service rsyslog start

chown root:root /etc/crontab
chmod og-rwx /etc/crontab
service cron start

tail -f /var/log/cron.log
