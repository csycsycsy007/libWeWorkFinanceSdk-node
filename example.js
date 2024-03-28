

const crypto = require('crypto');
const fs = require('fs');
const path = require('path');

const addon = require('./lib/wework.node');

const corpid = "替换成自己的corpid"
const secret = "替换成自己的secret"

async function work() {

    try {
        //清空 temp clearDir
        clearDir(path.resolve(__dirname, './temp'))

        //初始化
        const ret = addon.init(corpid, secret)
        if (ret != 0) {
            console.log("初始化失败：", ret)
            return
        }

        //获取会话记录
        const limit = 1000
        let seq = 0
        let hasMore = true
        while (hasMore) {
            const data = addon.getChatData(seq, limit, '', '', 60);
            const json = JSON.parse(data)
            if (json.errcode != 0) {
                log({ 'stack': json })
                hasMore = false
            }

            const chatdatas = json.chatdata
            let msgs = []
            for (var i = 0; i < chatdatas.length; i++) {
                const chatdata = chatdatas[i];

                const encrypt_random_key = chatdata.encrypt_random_key;
                const encrypt_chat_msg = chatdata.encrypt_chat_msg;

                const encrypt_key = decryptData(encrypt_random_key);
                const msg = addon.decryptdata(encrypt_key, encrypt_chat_msg);
                msgs.push(JSON.parse(msg));
            }

            //存入数据库
            await saveMsgs(msgs);

            seq += msgs.length;
            hasMore = msgs.length == limit ? true : false;
        }

    } catch (err) {
        log(err);
    }
}

function decryptData(encrypt_random_key, publickey_ver) {
    // base64解码
    const str1 = Buffer.from(encrypt_random_key, 'base64').toString('binary');

    // 读取私钥此处把 private.key 放到 lib 文件夹下  
    const privateKey = fs.readFileSync(path.resolve(__dirname, './lib/private.key'));

    const str2 = crypto.privateDecrypt({
        key: privateKey,
        padding: crypto.constants.RSA_PKCS1_PADDING
    }, Buffer.from(encrypt_random_key, 'base64')).toString();

    return str2;
}

async function saveMsgs(msgs) {

    for (let i in msgs) {
        let msg = msgs[i];

        //此处逻辑， 如果是媒体数据，需要存到自己的服务器或云服务器
        let md5sum = null;
        const allowedMsgTypes = ['image', 'emotion', 'voice', 'video', 'file'];
        if (allowedMsgTypes.includes(msg.msgtype)) {
            const msgData = msg[msg.msgtype];
            const sdkfileid = msgData.sdkfileid;
            md5sum = msgData.md5sum;
            const ret = addon.getMediaData(sdkfileid, '', '', 60, path.resolve(__dirname, `../libs/temp/${md5sum}`));

            //此处把上面刚写到 temp 目录下的文件传到自己的服务器
        }
        
        //此处同步到自己的数据库
    }
}


function clearDir(folderPath) {
    // 获取文件夹下的所有文件名
    fs.readdir(folderPath, (err, files) => {
        if (err) throw err;

        // 删除每个文件
        for (const file of files) {
            fs.unlinkSync(`${folderPath}/${file}`);
        }
    });
}


(async function worker() {
    await work();
})();