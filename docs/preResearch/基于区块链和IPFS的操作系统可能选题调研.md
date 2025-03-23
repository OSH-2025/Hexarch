# 基于区块链和IPFS的操作系统可能选题调研

# 李易 PB23111726


## 背景简介

### 区块链技术

区块链技术是以比特币为代表的数字加密货币体系的核心技术支撑，从狭义层面讲，区块链是按时间顺序将存储数据的区块以链条的方式重新组合起来的数据系统，它具备安全可靠。存储简单、可以自行验证数据，去中心化的特点。从广义层面来讲，区块链就是一种去中心化的基础架构与分布式计算范式。

### 以太坊

以太坊（Ethereum）是一个去中心化、开源并且具备智能合约功能的公共区块链平台。最早由维塔利克·布特林于2013年提出，具有智能合约，分布式应用程序，权益证明，等特点，以solidity为主要编程语言。

## IPFS

IPFS，是点到点的超媒体协议，是旨在实现文件的分布式存储、共享和持久化的网络传输协议。该传输协议以哈希为基础，在传输过程中，可保障网络平稳飞速地运行，其对数据的保密性更强，本质上是一种P2P(peer to peer)网络。



## 或许可选的主题

### 基于IPFS和区块链的分布式文件系统

使用IPFS设计文件操作系统，用于去中心化存储，使用区块链实现完整性和访问控制，与操作系统课程相关性较大。并且可以添加相关功能，例如：

- 数据来源保护

        使用区块链跟踪文件/数据历史记录和来源，保证透明度，提高安全性。

- 去中心化访问控制

        使用区块链智能合约来分散管理操作系统资源访问，这样可以实现去中心化，访问更准确高效，鲁棒性更高。

- 可搜索加密文件

        能够对加密文件进行关键字搜索

在可行性方面：

- 对于文件系统本身，可以选择模拟小规模模型，只需利用IPFS和区块链的基础知识。

- 对于数据来源保护，这还是主要涉及区块链知识

- 对于去中心化访问控制，涉及智能合约，可以模拟有限资源的访问

- 可搜索加密文件需要了解加密知识，涉及高级加密算法，稍微复杂但是可以简化模型



## 相关资源

- 一些论文：

    [基于IPFS区块链技术的工业互联网数据可信存储系统](https://pdf.hanspub.org/CSA20220500000_55907445.pdf)

    [基于区块链和IPFS的无人机数据监管及存储系统的设计](https://d.wanfangdata.com.cn/thesis/D02657265)

    [基于以太坊区块链和IPFS文件系统的溯源系统设计](https://d.wanfangdata.com.cn/thesis/ChhUaGVzaXNOZXdTMjAyNDA5MjAxNTE3MjUSCUQwMjUzNzQwMxoINHRvMXFoaTk%3D)

    [An Innovative IPFS-Based Storage Model for Blockchain](https://ieeexplore.ieee.org/document/8609675)

    [(PDF) Open Peer-to-Peer Systems over Blockchain and IPFS: an Agent Oriented Framework](https://www.researchgate.net/publication/325435917_Open_Peer-to-Peer_Systems_over_Blockchain_and_IPFS_an_Agent_Oriented_Framework)

    [A Blockchain and IPFS based framework for secure Research record keeping](https://acadpubl.eu/hub/2018-119-15/4/751.pdf)

- 相关资料

    [Blockchain Operating System: Overview and Examples](https://www.investopedia.com/terms/b/blockchain-operating-system.asp)

    [An open system to manage data without a central server | IPFS](https://ipfs.tech/)

    [Top 10 Trends in Blockchain Technology [2025] - GeeksforGeeks](https://www.geeksforgeeks.org/top-blockchain-technology-trends/)

    [基于区块链的星际文件系统点对点数据存储框架 - ScienceDirect](https://www.sciencedirect.com/science/article/pii/B9780128198162000022)

    [IPFS是什么？看这篇文章就够了 - 知乎](https://zhuanlan.zhihu.com/p/579018426)

    [区块链是什么](https://www.bilibili.com/video/BV1J7411Z7T9/?spm_id_from=333.337.search-card.all.click)

    [想知道比特币（和其他加密货币）的原理吗？]([【官方双语】想知道比特币（和其他加密货币）的原理吗？_哔哩哔哩_bilibili](https://www.bilibili.com/video/BV11x411i72w/?spm_id_from=333.337.search-card.all.click&vd_source=5f921bd7392238054539cbd9ba59c9e3))

- github相关开源项目
  
  - [ProvableHQ/snarkOS: A Decentralized Operating System for ZK Applications](https://github.com/ProvableHQ/snarkOS)
    
    一个基于Aleo区块链网络的去中心化操作系统，有点大，对设备要求有点高，可以看看
  
  - [Play-OS/PlayOS: 🖥 Web Operating System for mobile & desktop](https://github.com/Play-OS/PlayOS)
    
    一个基于Web的操作系统，支持与区块链等去中心化后端的集成


