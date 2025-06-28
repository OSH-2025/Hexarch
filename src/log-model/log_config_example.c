/**
 * FreeRTOS 日志模块配置示例
 * 
 * 这个文件展示了如何在编译时启用或禁用日志模块
 */

/*
 * 在 FreeRTOSConfig.h 中设置以下宏来控制日志模块：
 */

/* ========== 启用日志模块 ========== */
#define configUSE_LOG_MODULE    1

/*
 * 当 configUSE_LOG_MODULE = 1 时：
 * - 日志函数会正常工作
 * - 包含完整的日志实现代码
 * - 占用更多的 Flash 和 RAM 空间
 * - 提供完整的调试功能
 */

/* ========== 禁用日志模块 ========== */
// #define configUSE_LOG_MODULE    0

/*
 * 当 configUSE_LOG_MODULE = 0 时：
 * - 所有日志函数调用会被编译器优化掉
 * - 不占用额外的 Flash 和 RAM 空间
 * - 提高运行时性能
 * - 但失去调试信息
 */

/*
 * ========== 编译配置示例 ==========
 *   
 *  通过 Makefile 控制：
 *  make ENABLE_LOG=1    # 启用日志
 *  make ENABLE_LOG=0    # 禁用日志
 *    
 *  在 Makefile 中可以添加：
 *  ifeq ($(ENABLE_LOG), 1)
 *      CPPFLAGS += -DconfigUSE_LOG_MODULE=1
 *  else
 *      CPPFLAGS += -DconfigUSE_LOG_MODULE=0
 *  endif
 */

/*
 * ========== 代码示例 ==========
 */

/*
 * ========== 性能影响对比 ==========
 * 
 * 启用日志时：
 * - Flash 使用量：增加约 2-4KB（取决于实现复杂度）
 * - RAM 使用量：增加约 256-512 字节（缓冲区等）
 * - CPU 开销：每次日志调用约 10-50 微秒
 * 
 * 禁用日志时：
 * - Flash 使用量：几乎无增加（只有空宏定义）
 * - RAM 使用量：无增加
 * - CPU 开销：无开销（调用被优化掉）
 */
