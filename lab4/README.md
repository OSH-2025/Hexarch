本实验成功部署 Qwen2.5-VL-7B-Instruct (Q4_K_M)，测试线程数、prompt 长度和批处理大小的影响。最佳配置（`-t 8`, `-p 128`, `-b 64`）实现 7.42 t/s 吞吐量，困惑度 2.92 表明优秀语言建模能力。内存占用（~5.2GB）适配低配硬件。优化显著提升性能，满足实验要求。

相关文档已发布至CSDN：[CSDN链接](https://blog.csdn.net/liyi0658/article/details/149103960?fromshare=blogdetail&sharetype=blogdetail&sharerId=149103960&sharerefer=PC&sharesource=liyi0658&sharefrom=from_link)