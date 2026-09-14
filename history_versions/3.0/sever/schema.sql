-- 虚拟目录表（单表同时存用户根目录行 + 目录/文件行）
-- 如果还没有这个表，直接执行下面 CREATE；
-- 如果已经按旧结构建过表，执行后面的 ALTER 修正即可。

-- ===== 方案一：全新建表 =====
CREATE TABLE IF NOT EXISTS User (
    id       INT AUTO_INCREMENT PRIMARY KEY,
    name     VARCHAR(50)  NULL,          -- 用户名：只有“用户根目录行”填用户名，其余行填 NULL
    filename VARCHAR(50)  NULL,          -- 文件/目录名：根目录行为 NULL
    pre_id   INT          NULL,          -- 父目录行的 id（根目录行为 NULL）
    path     VARCHAR(100) NULL,          -- 从用户虚拟根目录开始的完整路径
    type     VARCHAR(10)  NULL,          -- 'dir' 或 'file'
    sha1     VARCHAR(40)  NULL,          -- 文件内容的 SHA1（目录行为 NULL），存储文件名即该哈希
    tomb     INT          NULL,          -- 0 正常 / 1 已删除
    password VARCHAR(128) NOT NULL,      -- 密码哈希（目录/文件行填 ''）
    salt     VARCHAR(30)  NULL,
    UNIQUE KEY uq_name (name)
);

-- ===== 方案二：已有旧表时，修正字段 =====
-- ALTER TABLE User MODIFY sha1 VARCHAR(40) NULL;
-- ALTER TABLE User MODIFY name VARCHAR(50) NULL;
-- 若上面把唯一索引弄丢了，再执行：
-- ALTER TABLE User ADD UNIQUE KEY uq_name (name);
