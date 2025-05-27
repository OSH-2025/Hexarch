extern crate libc;
use libc::*;

// portmacro里有部分类型的声明
use portmacro::*;

/**
 * 这个list的数据结构中的某些东西的定义很可能是错误的
 * 现阶段也只能是先写上了
 * 原因是rust的一些特性使得链表list的实现比较麻烦
 */

#[repr]
struct xLIST_ITEM{
    //如果字段不声明为 pub，则外部模块无法直接访问或修改它们：
    //这是AI告诉我的,我不确定
    pub xItemValue: portTickType,
    // FIXME(说实话如果要使用rust的安全性,下边这两个指针一定是不能这么定义的,只是现阶段只能先这么写着,可以参考一下去年学长的实现)
    pub pxNext: *mut xLIST_ITEM,
    pub pxPrevious: *mut xLIST_ITEM,
    pub pvOwner: *mut c_void,
    pub pvContainer: *mut c_void,
}

pub type xListItem = xLIST_ITEM; // for some reason lint wants this as two separate definitions

#[repr]
struct xMINI_LIST_ITEM{
    pub xItemValue: portTickType,
    pub pxNext: *mut xLIST_ITEM,
    pub pxPrevious: *mut xLIST_ITEM,
}

pub type xMiniListItem = xMINI_LIST_ITEM;

#[no_mangle]
pub struct xLIST{
    pub uxNumberOfItems: portBASE_TYPE_UNSIGNED,
    pub pxIndex: *mut xListItem,
    pub xListEnd: xMiniListItem,
}

pub type xList = xLIST;

impl xListItem{
    //对应了line 140 - 177的宏函数
    pub const fn new() -> Self{
        //TODO
    }

    // FIXME(我对这里的pxOwner类型存疑,暂时这么写)
    pub fn set_owner(&mut self, pxOwner: *mut c_void){
        //TODO
    }

    pub fn get_owner(self) -> *mut c_void{
        //TODO
    }

    pub fn set_value(&mut self, xValue: portTickType){
        //TODO
    }

    pub fn get_value(self) -> portTickType {
        //TODO
    }
}

impl xList{
    // TODO(参数有待商榷,返回值也不太确定)
    pub const fn new() -> Self{
        //TODO
    }

    //MEMO(self参数会获取所有权,实例会被销毁; &self是不可变借用,实例仍然可用; &mut self是可变借用, 实例仍然存在,但是可变借用同时只能存在一个)
    pub fn is_empty(&self) -> bool {
        //TODO
    }

    pub fn list_length(&self) -> portBASE_TYPE_UNSIGNED {
        //TODO
    }

    //获取下一个元素的所有者,并更新索引, c中这个宏函数自动处理了边界条件,所以这里也应该处理边界条件
    pub fn get_owner_of_the_next_entry(&mut self) -> *mut c_void {
        //TODO
    }

    pub fn get_owner_of_head_entry(&mut self) -> *mut c_void{
        //TODO
    }

    pub fn is_initialised(&self) -> bool {
        //TODO
    }

    pub fn is_contained_within(&self, pxListItem: &xListItem) -> bool {
        //TODO
    }
    
    //FIXME(关于xListItem究竟是&mut还是*mut的问题我并不是很明白,这个可能需要仔细讨论一下,目前的想法是rust内部还是都使用安全的类型,*这种类型可能是放在和c源码对应的函数API接口中吧)
    pub fn insert(&mut self, pxNewListItem: &mut xListItem){
        //TODO
    }

    pub fn insert_end(&mut self, pxNewListItem: &mut xListItem) {
        //TODO
    }

    //MEMO(关于所有权的那些事情又忘得差不多了,所以我也不是很确定这些参数的类型是否是正确的,感觉需要大家合在一起讨一下)
    pub fn remove(pxItemToRemove: &xListItem){
        //TODO
    }
}


// MEMO(下边这些是AI生成的,我复制下来是为了和大家讨论一下这件事情,就是如何安全的使用rust写好函数,然后暴露API给C的问题)
// === C 接口封装 ===

/// C 接口：创建新列表
#[no_mangle]
pub extern "C" fn xListCreate() -> *mut xList {
    Box::into_raw(Box::new(xList::new()))
}

/// C 接口：判断列表是否为空
#[no_mangle]
pub extern "C" fn xListIsEmpty(pxList: *mut xList) -> u8 {
    if pxList.is_null() {
        0
    } else {
        unsafe {
            (*pxList).is_empty() as u8
        }
    }
}

/// C 接口：获取列表长度
#[no_mangle]
pub extern "C" fn uxListLength(pxList: *mut xList) -> portBASE_TYPE_UNSIGNED {
    if pxList.is_null() {
        0
    } else {
        unsafe {
            (*pxList).list_length()
        }
    }
}

/// C 接口：插入新项到列表（对应 vListInsert）
#[no_mangle]
pub extern "C" fn vListInsert(pxList: *mut xList, pxNewListItem: *mut xListItem) {
    if pxList.is_null() || pxNewListItem.is_null() {
        return;
    }
    unsafe {
        (*pxList).insert(&mut *pxNewListItem);
    }
}

/// C 接口：插入新项到列表尾部（对应 vListInsertEnd）
#[no_mangle]
pub extern "C" fn vListInsertEnd(pxList: *mut xList, pxNewListItem: *mut xListItem) {
    if pxList.is_null() || pxNewListItem.is_null() {
        return;
    }
    unsafe {
        (*pxList).insert_end(&mut *pxNewListItem);
    }
}

/// C 接口：从列表中移除项（对应 vListRemove）
#[no_mangle]
pub extern "C" fn vListRemove(pxList: *mut xList, pxItemToRemove: *mut xListItem) {
    if pxList.is_null() || pxItemToRemove.is_null() {
        return;
    }
    unsafe {
        (*pxList).remove(&*pxItemToRemove);
    }
}