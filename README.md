# Hexarch

## 项目介绍
使用Rust重构C语言写的FreeRTOS，并添加FatFS文件系统拓展模块，增加其应用场景

## 成员

+ [PB23111723黄明昊](https://github.com/VideoBus66)（Group Leader）
+ [PB23111726李易](https://github.com/Leeyiiii)
+ [PB23111697刘思宇](https://github.com/MrKyomoto)
+ [PB23000055于皓翔](https://github.com/Parfait5)
+ [PB23111596王超然](https://github.com/cmdyc)
+ [PB23061139马文宇](https://github.com/LUNACY72)

![Group LOGO](https://github.com/OSH-2025/hexarch/blob/master/asset/group_logo.jpg)

## 项目进度
| 日期      | 事件     | 结果                                                    | 备注           |
| --------- | -------- | ------------------------------------------------------- | -------------- |
|2025.3.3-2025.3.12|各自调研，提出选题|于皓翔：[OS preresearch RAY](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E4%BA%8E%E7%9A%93%E7%BF%94_OS_preresearch_RAY.pdf)；<br>刘思宇：[FreeRTOS与FATFS的重构及结合](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E5%88%98%E6%80%9D%E5%AE%87_freeRTOS%20%26%20FatFs%E7%BB%93%E5%90%88%E4%BB%A5%E5%8F%8A%E9%87%8D%E6%9E%84%E7%9A%84%E5%8F%AF%E8%A1%8C%E6%80%A7%E6%8E%A2%E8%AE%A8.pdf)；<br>李易：[ROS(机器人操作系统)调研](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E6%9D%8E%E6%98%93-ROS.md)；<br>王超然：[虚拟化技术](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E7%8E%8B%E8%B6%85%E7%84%B6-%E8%99%9A%E6%8B%9F%E5%8C%96%E6%8A%80%E6%9C%AF.md)；<br>黄明昊：[Nginx负载均衡算法优化](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E9%BB%84%E6%98%8E%E6%98%8A-Nginx%E5%8F%8D%E5%90%91%E4%BB%A3%E7%90%86%E7%9A%84%E8%B4%9F%E8%BD%BD%E5%9D%87%E8%A1%A1%E7%AE%97%E6%B3%95%E4%BC%98%E5%8C%96%E4%B8%8E%E5%AE%9E%E7%8E%B0.pdf)；<br>马文宇：[AIOS](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E9%A9%AC%E6%96%87%E5%AE%87-AIOS.pdf)；|老师否定了改进Nginx相关的选题|
|2025.3.15|第一次讨论（线下）|对选题进行了初步的探讨，确定了几个基本方向，准备与老师讨论| [0315 1st discussion](https://github.com/OSH-2025/hexarch/blob/master/docs/discussion/0315_1st_discussion.md)|
|2025.3.17|与老师讨论|将“FreeRTOS与FATFS的重构及结合”与“基于区块链的分布式文件系统”作为初步的选题，准备调研之后从中确定选题||
|2025.3.18-2025.3.23|各自选择两者之一调研|于皓翔：[Blockchain_Research](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E4%BA%8E%E7%9A%93%E7%BF%94_Blockchain_Research.pdf)；<br>刘思宇：[FreeRTOS与FATFS的重构及结合](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E5%88%98%E6%80%9D%E5%AE%87_freeRTOS%20%26%20FatFs%E7%BB%93%E5%90%88%E4%BB%A5%E5%8F%8A%E9%87%8D%E6%9E%84%E7%9A%84%E5%8F%AF%E8%A1%8C%E6%80%A7%E6%8E%A2%E8%AE%A8.pdf);<br>李易：[基于区块链和IPFS的操作系统可能选题调研](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E6%9D%8E%E6%98%93-%E5%9F%BA%E4%BA%8E%E5%8C%BA%E5%9D%97%E9%93%BE%E5%92%8CIPFS%E7%9A%84%E6%93%8D%E4%BD%9C%E7%B3%BB%E7%BB%9F%E5%8F%AF%E8%83%BD%E9%80%89%E9%A2%98%E8%B0%83%E7%A0%94.md);<br>王超然：[区块链技术](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E7%8E%8B%E8%B6%85%E7%84%B6-%E5%8C%BA%E5%9D%97%E9%93%BE%E6%8A%80%E6%9C%AF.md);<br>黄明昊：[基于区块链的模型驱动文件系统](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E9%BB%84%E6%98%8E%E6%98%8A-%E5%9F%BA%E4%BA%8E%E5%8C%BA%E5%9D%97%E9%93%BE%E7%9A%84%E6%A8%A1%E5%9E%8B%E9%A9%B1%E5%8A%A8%E6%96%87%E4%BB%B6%E7%B3%BB%E7%BB%9F.pdf);<br>马文宇：[AIOS](https://github.com/OSH-2025/hexarch/blob/master/docs/preResearch/%E9%A9%AC%E6%96%87%E5%AE%87-AIOS.pdf)；||
|2025.3.23|第二次讨论（线下）|在两个不同的选题中，经过“激烈”的讨论，最终选定为：FreeRTOS与FatFs的结合及部分内核的Rust重构与添加| [0323 2nd discussion](https://github.com/OSH-2025/hexarch/blob/master/docs/discussion/0323_2nd_discussion.md)|
|2025.3.24|第三次讨论（线下）|我们在线下具体讨论了调研报告的内容和分工，具体分工为：<br>于皓翔 - C语言的局限性和Rust的优越性，C语言与Rust的混合编译；<br>刘思宇&&李易 - FreeRTOS文档&&源码阅读；<br>黄明昊&&马文宇 - FatFs文档&&源码阅读；<br>王超然 - FreeRTOS 系统可拓展模块及其应用场景||
|2025.3.29|第四次讨论（线下）|我们在研讨室根据这一周各自的调研内容，撰写了最终的调查报告|[researchReport](https://github.com/OSH-2025/hexarch/blob/master/docs/research%20report/%E8%B0%83%E7%A0%94%E6%8A%A5%E5%91%8A%20-%20FreeRTOS%E4%B8%8EFatFs%E7%9A%84%E7%BB%93%E5%90%88%E5%8F%8A%E9%83%A8%E5%88%86%E5%86%85%E6%A0%B8%E7%9A%84Rust%E9%87%8D%E6%9E%84%E4%B8%8E%E6%B7%BB%E5%8A%A0.pdf)|
|2025.3.30|第五次讨论(线上)|我们使用飞书协作平台开了一个线上会议,详细讨论了可行性报告的内容以及分工，具体分工如下：<br>李易：Rust改写C语言的可行性；刘思宇：Rust与C的混合编译；于皓翔, 王超然：嵌入式系统可拓展模块；黄明昊, 马文宇：FatFS 源码+文档分析||
|2025.4.5|第六次讨论(线下)|我们在2211空教室合作完成了可行性报告的撰写。同时，我们分析了后续工作：FreeRTOS和FatFS的结合部署；FatFS的单独部署；QEMU上部署FreeRTOS；rust全局分配器(这个任务旨在探讨重构方案)，并开始对部署FreeRTOS与FatFS的尝试|[可行性报告](https://github.com/OSH-2025/Hexarch/blob/master/docs/feasibility%20report/%E5%8F%AF%E8%A1%8C%E6%80%A7%E6%8A%A5%E5%91%8A.md)|
|2025.4.19|第七次讨论(线下)|我们分别完成了原版FreeRTOS与FatFS的单独部署，并在2211教室合作整理了中期汇报的文字内容，由李易同学完成了中期汇报PPT的整合与制作。<br>最后，我们讨论了Rust重构的具体步骤，初步确定为：从FreeRTOS数据结构与Rust的对应入手，在此基础上逐个函数地进行改性测试，由刘思宇、于皓翔同学进行数据结构对应工作，其余同学继续学习Rust、FreeRTOS架构，并尝试改写单个函数。<br>注：接下来10~15天，因期中考试、比赛等影响，进度暂停|[中期汇报](https://github.com/OSH-2025/Hexarch/tree/master/docs/Mid_report)|
|2025.5.17|第八次讨论(线下)|我们初步完成了数据结构的对应工作，并对Rust与FreeRTOS有了一定的了解。在此基础上，我们给出了模块与函数对外的接口，以及一些全局变量的定义。之后，我们继续讨论了这个月要进行的内核重构分工，每个人对应了一定数量的函数，并借助cursor生成并修改了部分代码||
|2025.5.24|第九次讨论(线下)|我们讨论了遇到的bug，分享了debug的经验，并对接下来几天大家需要攻克的事情，进行了更细致的重构与测试分工，为了保证进度，设置了严格的ddl。最后，我们上传了最新的工作。||
|2025.6.1|第十次讨论(线下)|我们为进一步优化，减少unsafe块进行了尝试与讨论，准备对C语言的双链表进行rust相关对应（数据结构对应部分依旧由刘思宇和于皓翔同学完成），其他unsafe场景如裸指针解引、union类型等由李易、王超然、马文宇、黄明昊调研并进行尝试改进||
|2025.6.5|第十一次讨论(线下)|初步完成了Rust改写以及重构部分，我们对于各个模块分别编写测试程序，进行正确性测试||
|2025.6.11|第十二次讨论(线下)|汇总进展与问题，接下来的下两周准备期末考试，考试结束后开始尝试Qemu测试与上板验证||
|2025.6.24|第十三次讨论(线下)|考试结束后进行Qemu测试，遇到了硬件接口问题||
|2025.6.26|第十四次讨论(线下)|解决了硬件接口问题，尝试为Rust版本添加模块||
|2025.6.28|第十五次讨论(线下)|Rust版本添加模块结束，开始期末答辩ppt相关工作，答辩分工如下：<br>文件模块：马文宇；日志模块：王超然；C重构：于皓翔；Rust重构：刘思宇，李易；介绍与总结：黄明昊||
|2025.7.2|第十六次讨论(线下)|完成最终的大作业报告，延续答辩的分工|[期末报告](https://github.com/OSH-2025/Hexarch/blob/master/docs/final_report/FinalReport.md)|
