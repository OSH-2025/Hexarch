#![no_std]
use crate::portable::{portBASE_TYPE, portMAX_DELAY, portTickType, unsigned_portBASE_TYPE};
use core::ffi::c_void;
use core::ptr::NonNull;
use core::sync::atomic::{AtomicPtr, Ordering};

// List constants
pub const listLIST_ITEM_CONTAINER_OBJECT: *mut c_void = core::ptr::null_mut();
pub const listLIST_ITEM_CONTAINER_TYPE: *mut c_void = core::ptr::null_mut();
pub const listLIST_ITEM_CONTAINER_VALUE: portTickType = 0;

#[repr(C)]
pub struct xListItem {
    pub xItemValue: portTickType,
    pub pxNext: AtomicPtr<xListItem>,
    pub pxPrevious: AtomicPtr<xListItem>,
    pub pvOwner: *mut c_void,
    pub pvContainer: *mut c_void,
}

#[repr(C)]
pub struct xMiniListItem {
    pub xItemValue: portTickType,
    pub pxNext: AtomicPtr<xListItem>,
    pub pxPrevious: AtomicPtr<xListItem>,
}

#[repr(C)]
pub struct xList {
    pub uxNumberOfItems: unsigned_portBASE_TYPE,
    pub pxIndex: AtomicPtr<xListItem>,
    pub xListEnd: xMiniListItem,
}

// Helper function to convert xMiniListItem to xListItem
unsafe fn mini_to_list_item(mini: &mut xMiniListItem) -> *mut xListItem {
    core::mem::transmute(mini)
}

// Helper function to convert xListItem to xMiniListItem
unsafe fn list_item_to_mini(item: &mut xListItem) -> *mut xMiniListItem {
    core::mem::transmute(item)
}

// Helper function to check if a list item is valid
unsafe fn prvIsListItemValid(pxItem: *const xListItem) -> bool {
    !pxItem.is_null() && (*pxItem).pvContainer != core::ptr::null_mut()
}

// Helper function to check if a list is valid
unsafe fn prvIsListValid(pxList: *const xList) -> bool {
    !pxList.is_null() && (*pxList).pxIndex.load(Ordering::SeqCst) != core::ptr::null_mut()
}

#[no_mangle]
pub extern "C" fn vListInitialise(pxList: *mut xList) {
    let list = unsafe { &mut *pxList };

    // Initialize the list index to point to the list end
    list.pxIndex.store(
        unsafe { mini_to_list_item(&mut list.xListEnd) },
        Ordering::SeqCst,
    );

    // Set the list end value to maximum possible value
    list.xListEnd.xItemValue = portMAX_DELAY;

    // Initialize the list end pointers to point to itself
    let end_ptr = unsafe { mini_to_list_item(&mut list.xListEnd) };
    list.xListEnd.pxNext.store(end_ptr, Ordering::SeqCst);
    list.xListEnd.pxPrevious.store(end_ptr, Ordering::SeqCst);

    // Initialize the number of items to 0
    list.uxNumberOfItems = 0;
}

#[no_mangle]
pub extern "C" fn vListInitialiseItem(pxItem: *mut xListItem) {
    let item = unsafe { &mut *pxItem };
    item.pvContainer = core::ptr::null_mut();
}

#[no_mangle]
pub extern "C" fn vListInsertEnd(pxList: *mut xList, pxNewListItem: *mut xListItem) {
    let list = unsafe { &mut *pxList };
    let new_item = unsafe { &mut *pxNewListItem };
    let index = unsafe { &mut *list.pxIndex.load(Ordering::SeqCst) };

    // Insert the new item after the current index
    new_item
        .pxNext
        .store(index.pxNext.load(Ordering::SeqCst), Ordering::SeqCst);
    new_item
        .pxPrevious
        .store(list.pxIndex.load(Ordering::SeqCst), Ordering::SeqCst);
    unsafe {
        (*index.pxNext.load(Ordering::SeqCst))
            .pxPrevious
            .store(new_item as *mut _, Ordering::SeqCst);
        index.pxNext.store(new_item as *mut _, Ordering::SeqCst);
    }
    list.pxIndex.store(new_item as *mut _, Ordering::SeqCst);

    // Set the container pointer
    new_item.pvContainer = pxList as *mut _;

    // Increment the number of items
    list.uxNumberOfItems += 1;
}

#[no_mangle]
pub extern "C" fn vListInsert(pxList: *mut xList, pxNewListItem: *mut xListItem) {
    let list = unsafe { &mut *pxList };
    let new_item = unsafe { &mut *pxNewListItem };
    let value_of_insertion = new_item.xItemValue;

    let mut px_iterator = if value_of_insertion == portMAX_DELAY {
        unsafe { &mut *list.xListEnd.pxPrevious.load(Ordering::SeqCst) }
    } else {
        let mut current = unsafe { &mut *list.xListEnd.pxNext.load(Ordering::SeqCst) };
        while unsafe { (*current.pxNext.load(Ordering::SeqCst)).xItemValue } <= value_of_insertion {
            current = unsafe { &mut *current.pxNext.load(Ordering::SeqCst) };
        }
        current
    };

    // Insert the new item
    new_item
        .pxNext
        .store(px_iterator.pxNext.load(Ordering::SeqCst), Ordering::SeqCst);
    unsafe {
        (*px_iterator.pxNext.load(Ordering::SeqCst))
            .pxPrevious
            .store(new_item as *mut _, Ordering::SeqCst);
    }
    new_item
        .pxPrevious
        .store(px_iterator as *mut _, Ordering::SeqCst);
    px_iterator
        .pxNext
        .store(new_item as *mut _, Ordering::SeqCst);

    // Set the container pointer
    new_item.pvContainer = pxList as *mut _;

    // Increment the number of items
    list.uxNumberOfItems += 1;
}

#[no_mangle]
pub extern "C" fn vListRemove(pxItemToRemove: *mut xListItem) {
    let item = unsafe { &mut *pxItemToRemove };
    let list = unsafe { &mut *(item.pvContainer as *mut xList) };

    // Remove the item from the list
    unsafe {
        (*item.pxNext.load(Ordering::SeqCst))
            .pxPrevious
            .store(item.pxPrevious.load(Ordering::SeqCst), Ordering::SeqCst);
        (*item.pxPrevious.load(Ordering::SeqCst))
            .pxNext
            .store(item.pxNext.load(Ordering::SeqCst), Ordering::SeqCst);
    }

    // Update the index if necessary
    if list.pxIndex.load(Ordering::SeqCst) == pxItemToRemove {
        list.pxIndex
            .store(item.pxPrevious.load(Ordering::SeqCst), Ordering::SeqCst);
    }

    // Clear the container pointer
    item.pvContainer = core::ptr::null_mut();

    // Decrement the number of items
    list.uxNumberOfItems -= 1;
}

// Helper macros converted to functions
#[inline]
pub fn listSET_LIST_ITEM_OWNER(pxListItem: *mut xListItem, pxOwner: *mut c_void) {
    unsafe {
        (*pxListItem).pvOwner = pxOwner;
    }
}

#[inline]
pub fn listGET_LIST_ITEM_OWNER(pxListItem: *const xListItem) -> *mut c_void {
    unsafe { (*pxListItem).pvOwner }
}

#[inline]
pub fn listSET_LIST_ITEM_VALUE(pxListItem: *mut xListItem, xValue: portTickType) {
    unsafe {
        (*pxListItem).xItemValue = xValue;
    }
}

#[inline]
pub fn listGET_LIST_ITEM_VALUE(pxListItem: *const xListItem) -> portTickType {
    unsafe { (*pxListItem).xItemValue }
}

#[inline]
pub fn listGET_ITEM_VALUE_OF_HEAD_ENTRY(pxList: *const xList) -> portTickType {
    unsafe { (*(*pxList).xListEnd.pxNext.load(Ordering::SeqCst)).xItemValue }
}

#[inline]
pub fn listLIST_IS_EMPTY(pxList: *const xList) -> bool {
    unsafe { (*pxList).uxNumberOfItems == 0 }
}

#[inline]
pub fn listCURRENT_LIST_LENGTH(pxList: *const xList) -> unsigned_portBASE_TYPE {
    unsafe { (*pxList).uxNumberOfItems }
}

#[inline]
pub fn listGET_OWNER_OF_HEAD_ENTRY(pxList: *const xList) -> *mut c_void {
    unsafe { (*(*pxList).xListEnd.pxNext.load(Ordering::SeqCst)).pvOwner }
}

#[inline]
pub fn listGET_OWNER_OF_NEXT_ENTRY(pxTCB: *mut *mut c_void, pxList: *mut xList) {
    let list = unsafe { &mut *pxList };

    // Increment the index to the next item
    list.pxIndex.store(
        unsafe {
            (*list.pxIndex.load(Ordering::SeqCst))
                .pxNext
                .load(Ordering::SeqCst)
        },
        Ordering::SeqCst,
    );

    // Skip the list end marker if we've reached it
    if list.pxIndex.load(Ordering::SeqCst) == unsafe { mini_to_list_item(&mut list.xListEnd) } {
        list.pxIndex.store(
            unsafe {
                (*list.pxIndex.load(Ordering::SeqCst))
                    .pxNext
                    .load(Ordering::SeqCst)
            },
            Ordering::SeqCst,
        );
    }

    // Return the owner of the next item
    unsafe {
        *pxTCB = (*list.pxIndex.load(Ordering::SeqCst)).pvOwner;
    }
}

#[inline]
pub fn listIS_CONTAINED_WITHIN(pxList: *const xList, pxListItem: *const xListItem) -> bool {
    unsafe { (*pxListItem).pvContainer == pxList as *mut _ }
}

#[inline]
pub fn listGET_HEAD_ENTRY(pxList: *const xList) -> *mut xListItem {
    unsafe { (*pxList).xListEnd.pxNext.load(Ordering::SeqCst) }
}

#[inline]
pub fn listGET_NEXT(pxListItem: *const xListItem) -> *mut xListItem {
    unsafe { (*pxListItem).pxNext.load(Ordering::SeqCst) }
}

#[inline]
pub fn listGET_END_MARKER(pxList: *const xList) -> *mut xListItem {
    unsafe { mini_to_list_item(&mut (*pxList).xListEnd) }
}

#[inline]
pub fn listLIST_ITEM_CONTAINER(pxListItem: *const xListItem) -> *mut xList {
    unsafe { (*pxListItem).pvContainer as *mut xList }
}

#[inline]
pub fn listLIST_IS_INITIALISED(pxList: *const xList) -> bool {
    unsafe { !(*pxList).pxIndex.load(Ordering::SeqCst).is_null() }
}

// Additional helper functions
#[inline]
pub fn listGET_ITEM_VALUE_OF_HEAD_ENTRY_FROM_ISR(pxList: *const xList) -> portTickType {
    unsafe { (*(*pxList).xListEnd.pxNext.load(Ordering::SeqCst)).xItemValue }
}

#[inline]
pub fn listGET_OWNER_OF_HEAD_ENTRY_FROM_ISR(pxList: *const xList) -> *mut c_void {
    unsafe { (*(*pxList).xListEnd.pxNext.load(Ordering::SeqCst)).pvOwner }
}

#[inline]
pub fn listGET_NEXT_FROM_ISR(pxListItem: *const xListItem) -> *mut xListItem {
    unsafe { (*pxListItem).pxNext.load(Ordering::SeqCst) }
}

#[inline]
pub fn listGET_END_MARKER_FROM_ISR(pxList: *const xList) -> *mut xListItem {
    unsafe { mini_to_list_item(&mut (*pxList).xListEnd) }
}
