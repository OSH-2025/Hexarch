#![no_std]
#![allow(non_snake_case)]
extern crate alloc;

extern crate libc;
use libc::*;

// portmacro里有部分类型的声明
use portmacro::*;
use alloc::sync::{Arc,Weak};
use crate::rwlock::*;

use alloc::format;
use core::clone::Clone;
use core::default::Default;
/**
 * 这个list的数据结构中的某些东西的定义很可能是错误的
 * 现阶段也只能是先写上了
 * 原因是rust的一些特性使得链表list的实现比较麻烦
 */

pub type linkItem = Arc<RwLock<xLIST_ITEM>>;
pub type weakLinkItem = Weak<RwLock<xLIST_ITEM>>;

pub type linkList = Arc<RwLock<xLIST>>;
pub type weakLinkList = Weak<RwLock<xLIST>>;

pub struct xLIST_ITEM{
    //如果字段不声明为 pub，则外部模块无法直接访问或修改它们：
    //这是AI告诉我的,我不确定
    pub xItemValue: portTickType,
    pub pxNext: weakLinkItem,
    pub pxPrevious: weakLinkItem,
    pub pvOwner: Weak<RwLock<TaskControlBlock>>,
    pub pvContainer: Weak<RwLock<xLIST>>,
}

pub type xListItem = xLIST_ITEM; // for some reason lint wants this as two separate definitions

// pub struct xMINI_LIST_ITEM{
//     pub xItemValue: portTickType,
//     pub pxNext: weakLinkItem,
//     pub pxPrevious: weakLinkItem,
// }

// pub type xMiniListItem = xMINI_LIST_ITEM;

#[derive(Clone)]
pub struct xLIST{
    pub uxNumberOfItems: portBASE_TYPE_UNSIGNED,
    pub pxIndex: weakLinkItem,
    pub xListEnd: linkItem,
}

pub type xList = xLIST;

impl xListItem{
    //对应了line 140 - 177的宏函数
    pub fn new() -> Self{
        xListItem{
            // xItemValue是链表辅助排序的值,默认设为最大
            xItemValue: portMAX_DELAY,
            pxNext: Default::default(),
            pxPrevious: Default::default(),
            pvOwner: Default::default(),
            pvContainer: Default::default(),
        }
    }

    // FIXME(我对这里的pxOwner类型存疑,暂时这么写)
    pub fn set_owner(&mut self, pxOwner: Arc<RwLock<TaskControlBlock>>){
        self.pvOwner = Arc::downgrade(&pxOwner);
    }

    pub fn get_owner(&self) -> Weak<RwLock<TaskControlBlock>>{
        self.pvOwner
    }

    pub fn set_value(&mut self, xValue: portTickType){
        self.xItemValue = xValue;
    }

    pub fn get_value(self) -> portTickType {
        self.xItemValue
    }

    pub fn set_container(&mut self,pxContainer: Arc<RwLock<xLIST>>){
        self.pvContainer = Arc::downgrade(&pxContainer);
    }

    pub fn remove(&mut self, xLink: weakLinkItem) -> portBASE_TYPE_UNSIGNED{
        let list = self
            .pvContainer
            .upgrade()
            .unwrap_or_else(||panic!("Container not set"));

        let ret = list
            .write()
            .remove_item(&self,xLink);
        set_list_item_next(&self.pxPrevious, Weak::clone(&self.pxNext));
        set_list_item_prev(&self.pxNext,Weak::clone(&self.pxPrevious));
        self.pvContainer = Weak::new();
        ret
    }
}

// 从原始listItem获取Arc包裹后的
pub fn new_list_item(xItemValue: portTickType) -> Arc<RwLock<xLIST_ITEM>>{
    let mut rawListItem = xListItem::new();
    rawListItem.set_value(xItemValue);
    let listItem : Arc<RwLock<xLIST_ITEM>> = Arc::new(RwLock::new(rawListItem));
    listItem
}

impl xList{
    // TODO(参数有待商榷,返回值也不太确定)
    pub fn new() -> Self{
        let xListEnd: linkItem = Arc::new(RwLock::new(xListItem::new()));
        //最后一个节点的pxNext和pxPrevious指向自身
        (*xListEnd.wirte()).pxNext = Arc::downgrade(&xListEnd);
        (*xListEnd.wirte()).pxPrevious = Arc::downgrade(&xListEnd);

        xLIST{
            uxNumberOfItems:0,
            pxIndex: Arc::downgrade(&xListEnd),
            xListEnd: xListEnd,
        }
    }

    //MEMO(self参数会获取所有权,实例会被销毁; &self是不可变借用,实例仍然可用; &mut self是可变借用, 实例仍然存在,但是可变借用同时只能存在一个)
    pub fn is_empty(&self) -> bool {
        self.uxNumberOfItems == 0
    }

    pub fn list_length(&self) -> portBASE_TYPE_UNSIGNED {
        self.uxNumberOfItems
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
    
    pub fn insert(&mut self, pxNewListItem: weakLinkItem){
        let xItemValue = get_weak_item_value(&&pxNewListItem);

        let itemToInsert = if xItemValue == portMAX_DELAY{
            get_list_item_prev(&Arc::downgrade(&self.xListEnd))
        }else{
            let mut iterator = Arc::downgrade(&self.xListEnd);
            loop{
                let next = get_list_item_next(&iterator);
                if get_weak_item_value(&next) > xItemValue{
                    break iterator;
                }
                iterator = next;
            }
        };
        let prev = Weak::clone(&itemToInsert);
        let next = get_list_item_next(&itemToInsert);

        //BUG(这里的链表关系存疑)
        set_list_item_next(&pxNewListItem,Weak::clone(&next));
        set_list_item_prev(&pxNewListItem,Weak::clone(&prev));
        set_list_item_next(&prev, Weak::clone(&pxNewListItem));
        set_list_item_prev(&next, Weak::clone(&pxNewListItem));

        self.uxNumberOfItems += 1;
    }

    pub fn insert_end(&mut self, pxNewListItem: weakLinkItem) {
        //TODO
    }

    //MEMO(关于所有权的那些事情又忘得差不多了,所以我也不是很确定这些参数的类型是否是正确的,感觉需要大家合在一起讨一下)
    pub fn remove(pxItemToRemove: &xListItem){
        //TODO
    }
}

fn set_list_item_next(item: &weakLinkItem, next:weakLinkItem){
    let ownedItem: Arc<RwLock<xLIST_ITEM>> = item
        .upgrade()
        .unwrap_or_else(|| panic!("List item is None"));
    (*ownedItem.write()).pxNext = next;
}

fn set_list_item_prev(item: &weakLinkItem, prev:weakLinkItem){
    let ownedItem: Arc<RwLock<xLIST_ITEM>> = item
        .upgrade()
        .unwrap_or_else(|| panic!("List item is None"));
    (*ownedItem.write()).pxPrevious = prev;
}

fn get_list_item_prev(item: &weakLinkItem) -> weakLinkItem{
    let ownedItem = item
        .upgrade()
        .unwrap_or_else(|| panic!("List item is None"));
    let prev = Weak::clone(&(*ownedItem.read()).pxPrevious);
    prev
}

fn get_list_item_next(item: &weakLinkItem) -> weakLinkItem{
    let ownedItem = item
        .upgrade()
        .unwrap_or_else(|| panic!("List item is None"));
    let next = Weak::clone(&(*ownedItem.read()).pxPrevious);
    next
}

pub fn listGET_LIST_ITEM_VALUE(item: &linkItem) -> portTickType{
    (*item.read()).xItemValue
}

pub fn 


