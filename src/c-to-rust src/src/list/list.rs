#![allow(non_snake_case)]
//! Bidirectional linked list Definition and API
extern crate alloc;
use crate::portable::portmacro::*;
use crate::portable::riscv_virt::*;
use crate::portable::*;
use crate::tasks::*;
use alloc::format;
use alloc::sync::{Arc, Weak};
use core::clone::Clone;
use core::default::Default;
use spin::RwLock;
pub type WeakListItemLink = Weak<RwLock<XListItem>>;
pub type WeakListLink = Weak<RwLock<XList>>;
pub type ArcListLink = Arc<RwLock<XList>>;
pub type ListItemLink = Arc<RwLock<XListItem>>;
pub type WeakListItemOwnerLink = Weak<RwLock<TCB_t>>;

use alloc::string;
use core::option::Option;

#[derive(Debug)]
pub struct XListItem {
    /// Used to help arrange nodes in order.
    pub xItemValue: TickType,
    /// Point to the next linked list item.
    pub pxNext: WeakListItemLink,
    /// Point to the previous linked list item.
    pub px_previous: WeakListItemLink,
    /// Point to the linked list where the node is located, and point to the kernel object that owns the node.
    pub pvOwner: WeakListItemOwnerLink,
    /// Point to the linked list where the node is located.
    pub pxContainer: WeakListLink,
    pub pvOwnerC: usize,
}
pub type ListItemT = XListItem;
impl XListItem {
    pub fn new(value: TickType) -> Self {
        ListItemT {
            xItemValue: value,
            pxNext: Default::default(),
            px_previous: Default::default(),
            pvOwner: Default::default(),
            pxContainer: Default::default(),
            pvOwnerC: Default::default(),
        }
    }
}
//链表节点初始化
impl Default for ListItemT {
    fn default() -> Self {
        ListItemT {
            xItemValue: 0,
            pxNext: Default::default(),
            px_previous: Default::default(),
            pvOwner: Default::default(),
            pxContainer: Default::default(),
            pvOwnerC: Default::default(),
        }
    }
}
//#[derive(Debug)]
#[derive(Clone, Debug)]
pub struct XList {
    pub uxNumberOfItems: UBaseType,
    pxIndex: WeakListItemLink,
    xListEnd: Arc<RwLock<ListItemT>>,
}
pub type ListT = XList;
//链表根节点初始化
impl Default for ListT {
    fn default() -> Self {
        //得到一个list_end 然后设置其辅助排序值 并将其next和pre指向自身
        let xListEnd = Arc::new(RwLock::new(XListItem::default()));
        (xListEnd).write().xItemValue = PORT_MAX_DELAY;
        (xListEnd).write().pxNext = Arc::downgrade(&xListEnd);
        (xListEnd).write().px_previous = Arc::downgrade(&xListEnd);
        ListT {
            uxNumberOfItems: 0,
            pxIndex: Arc::downgrade(&xListEnd),
            xListEnd: xListEnd,
        }
    }
}

/// 设置目标节点的前驱节点。
pub fn listItemSetPre(item: &WeakListItemLink, pre: WeakListItemLink) {
    item.upgrade().unwrap().write().px_previous = pre;
}

/// 设置目标节点的后继节点。
pub fn listItemSetNext(item: &WeakListItemLink, next: WeakListItemLink) {
    (item.upgrade().unwrap()).write().pxNext = next;
}

/// 获取目标节点的前驱节点。
pub fn listItemGetPre(item: &WeakListItemLink) -> WeakListItemLink {
    let pre = Weak::clone(&(item.upgrade().unwrap()).read().px_previous);
    pre
}

/// 获取目标节点的后继节点。
pub fn listItemGetNext(item: &WeakListItemLink) -> WeakListItemLink {
    let next = Weak::clone(&(item.upgrade().unwrap()).read().pxNext);
    next
}

/// 设置节点所属的链表容器。
pub fn listItemSetContainer(item: &WeakListItemLink, container: WeakListLink) {
    (item.upgrade().unwrap()).write().pxContainer = container;
}

/// 获取节点的值（用于排序）。
pub fn listItemGetValue(item: &ListItemLink) -> TickType {
    let value = (item).read().xItemValue;
    value
}

/// 设置节点的值（用于排序）。
pub fn listItemSetValue(item: &ListItemLink, x_value: TickType) {
    (item).write().xItemValue = x_value;
}

/// 获取链表头部的第一个有效节点。
pub fn listGetHeadEntry(px_list: &ArcListLink) -> WeakListItemLink {
    let entry = Weak::clone(&((px_list).read().xListEnd).read().pxNext);
    entry
}

/// 获取链表的结束标记节点。
pub fn listGetEndMarker(px_list: &ArcListLink) -> WeakListItemLink {
    let entry = Arc::downgrade(&(px_list).read().xListEnd);
    entry
}

/// 获取节点所属的链表容器。
pub fn listItemGetContainer(item: &WeakListItemLink) -> WeakListLink {
    let container = Weak::clone(&(item.upgrade().unwrap()).read().pxContainer);
    container
}

/// 设置节点的拥有者（通常为任务控制块）。
pub fn listItemSetOwner(item: &ListItemLink, owner: WeakListItemOwnerLink) {
    (item).write().pvOwner = Weak::clone(&owner);
}

/// 获取节点的拥有者（通常为任务控制块）。
pub fn listItemGetOwner(item: &WeakListItemLink) -> WeakListItemOwnerLink {
    let owner = Weak::clone(&(item.upgrade().unwrap()).read().pvOwner);
    owner
}

/// 获取节点的拥有者（以C指针形式保存）。
pub fn listItemGetOwnerC(item: &WeakListItemLink) -> Option<TaskHandle_t> {
    let owner = item.upgrade().unwrap().read().pvOwnerC;
    if owner == 0 {
        return None;
    } else {
        return Some(unsafe { Arc::from_raw(owner as *const RwLock<tskTaskControlBlock>) });
    }
}

/// 获取链表中节点数量。
pub fn listGetNumItems(px_list: &WeakListLink) -> UBaseType {
    let num = (px_list.upgrade().unwrap()).read().uxNumberOfItems;
    num
}

/// 获取链表当前索引节点。
pub fn listGetpxIndex(px_list: &WeakListLink) -> WeakListItemLink {
    let pxIndex = Weak::clone(&(px_list.upgrade().unwrap()).read().pxIndex);
    pxIndex
}

/// 设置链表当前索引节点。
pub fn listSetpxIndex(px_list: &WeakListLink, item: WeakListItemLink) {
    (px_list.upgrade().unwrap()).write().pxIndex = item;
}

/// 判断链表是否为空。
pub fn listIsEmpty(px_list: &ArcListLink) -> bool {
    (px_list).read().uxNumberOfItems == 0
}

/// 获取链表中节点数量。
pub fn listCurrentListLength(px_list: &ArcListLink) -> UBaseType {
    (px_list).read().uxNumberOfItems
}

/// 获取链表下一个节点的拥有者，并移动索引。
pub fn listGetOwnerOfNextEntry(px_list: &ArcListLink) -> WeakListItemOwnerLink {
    //add index and return owner
    let owner = px_list.write().getOwnerOfNextEntry();
    owner
}

/// 获取链表头部节点的拥有者，不改变索引。
pub fn listGetOwnerOfHeadEntry(px_list: &ArcListLink) -> WeakListItemOwnerLink {
    let owner = px_list.write().getOwnerOfHeadEntry();
    owner
}

/// 获取链表头部节点的C标准拥有者，不改变索引。
pub fn listGetOwnerOfHeadEntryC(px_list: &ArcListLink) -> Option<TaskHandle_t> {
    let ret = px_list.write().getOwnerOfHeadEntryC();
    ret
}

impl ListT {
    /// Insert target item into end of list.
    pub fn insert_end(&mut self, px_new_list_item: WeakListItemLink) {
        // 获取当前索引的前驱节点
        let pxIndex_pre = listItemGetPre(&self.pxIndex);
        listItemSetNext(&px_new_list_item, Weak::clone(&self.pxIndex));
        listItemSetPre(&px_new_list_item, Weak::clone(&pxIndex_pre));
        listItemSetNext(&pxIndex_pre, Weak::clone(&px_new_list_item));
        listItemSetPre(&self.pxIndex, Weak::clone(&px_new_list_item));
        self.uxNumberOfItems += 1;
    }

    /// Insert target item into list in ascending order of value. <br>
    /// If target item has value==PORT_MAX_DELAY, insert to list end. <br>
    /// If list is not already in order, insert position is not guaranteed.
    pub fn insert(&mut self, px_new_list_item: WeakListItemLink) {
        let x_value_of_insertion = listItemGetValue(&Weak::upgrade(&px_new_list_item).unwrap());

        let px_iterator = if x_value_of_insertion == PORT_MAX_DELAY {
            // 如果是最大值，插入到链表末尾
            listItemGetPre(&(Arc::downgrade(&self.xListEnd)))
        } else {
            // 否则按值查找插入点
            let mut iterator = Arc::downgrade(&self.xListEnd);
            loop {
                iterator = listItemGetNext(&iterator);
                let value = listItemGetValue(&Weak::upgrade(&iterator).unwrap());

                if value >= x_value_of_insertion {
                    break;
                }
            }
            iterator
        };

        listItemSetNext(&px_new_list_item, Weak::clone(&px_iterator));
        listItemSetPre(
            &px_new_list_item,
            Weak::clone(&listItemGetPre(&px_iterator)),
        );
        listItemSetNext(
            &listItemGetPre(&px_iterator),
            Weak::clone(&px_new_list_item),
        );
        listItemSetPre(&px_iterator, Weak::clone(&px_new_list_item));
        self.uxNumberOfItems += 1;
    }

    /// Get owner of next entry in list. <br>
    /// Move current index to next item.
    pub fn getOwnerOfNextEntry(&mut self) -> WeakListItemOwnerLink {
        self.pxIndex = listItemGetNext(&self.pxIndex);
        if Weak::ptr_eq(&self.pxIndex, &Arc::downgrade(&self.xListEnd)) {
            self.pxIndex = listItemGetNext(&self.pxIndex);
        }

        let owner = Weak::clone(&self.pxIndex.upgrade().unwrap().read().pvOwner);
        owner
    }

    /// Get owner of head entry in list.
    /// 获取链表头部节点的拥有者。
    pub fn getOwnerOfHeadEntry(&mut self) -> WeakListItemOwnerLink {
        let end = self.xListEnd.read();
        let target: &WeakListItemLink = &(end.pxNext);

        listItemGetOwner(target)
    }

    /// Get owner stored in C standard of head entry in list.
    pub fn getOwnerOfHeadEntryC(&mut self) -> Option<TaskHandle_t> {
        let end = self.xListEnd.read();
        let target: ListItemLink = end.pxNext.upgrade().unwrap();
        let owner = target.read().pvOwnerC;
        if owner == 0 {
            return None;
        } else {
            // 通过C指针恢复Arc
            return Some(unsafe { Arc::from_raw(owner as *const RwLock<tskTaskControlBlock>) });
        }
    }
}

//=====================对外接口=====================

/// 将节点插入链表末尾。
pub fn vListInsertEnd(px_list: &ArcListLink, px_new_list_item: &ListItemLink) {
    px_list.write().insert_end(Arc::downgrade(px_new_list_item));

    px_new_list_item.write().pxContainer = Arc::downgrade(&px_list);
}

/// 按值升序将节点插入链表。
pub fn vListInsert(px_list: &ArcListLink, px_new_list_item: &ListItemLink) {
    px_list.write().insert(Arc::downgrade(px_new_list_item));

    px_new_list_item.write().pxContainer = Arc::downgrade(&px_list);
}

/// 从链表中移除节点，返回移除后的节点数。
pub fn uxListRemove(px_item_to_remove: WeakListItemLink) -> UBaseType {
    let px_list = listItemGetContainer(&px_item_to_remove);
    match px_list.upgrade() {
        Some(x) => {}
        None => {
            return 0;
        }
    }
    listItemSetPre(
        &listItemGetNext(&px_item_to_remove),
        listItemGetPre(&px_item_to_remove),
    );
    listItemSetNext(
        &listItemGetPre(&px_item_to_remove),
        listItemGetNext(&px_item_to_remove),
    );
    if Weak::ptr_eq(&px_item_to_remove, &listGetpxIndex(&px_list)) {
        listSetpxIndex(
            &px_list,
            Weak::clone(&listItemGetPre(&px_item_to_remove)),
        );
    }

    (px_list.upgrade().unwrap()).write().uxNumberOfItems -= 1;
    listItemSetContainer(&px_item_to_remove, Default::default());
    listGetNumItems(&px_list)
}

/// 判断节点是否属于指定链表。
pub fn listIsContainedWithin(px_list: &ArcListLink, px_new_list_item: &ListItemLink) -> bool {
    let temp = Arc::downgrade(px_list);
    temp.ptr_eq(&px_new_list_item.read().pxContainer)
}

/// 获取链表头部节点的值。
pub fn listGetValueOfHeadEntry(px_list: &ArcListLink) -> UBaseType {
    listItemGetValue(&Weak::upgrade(&listGetHeadEntry(px_list)).unwrap())
}
