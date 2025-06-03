// From https://github.com/ytakano/synctools/blob/main/src/rwlock.rs

use core::marker::PhantomData;

#[cfg(not(loom))]
use core::{
    cell::UnsafeCell,
    hint,
    ops::{Deref, DerefMut},
    sync::atomic::{AtomicUsize, Ordering},
};

#[cfg(loom)]
use loom::{
    cell::UnsafeCell,
    hint,
    sync::atomic::{AtomicUsize, Ordering},
};

pub struct RwLock<T> {
    state: AtomicUsize,
    writer_wake_counter: AtomicUsize,
    data: UnsafeCell<T>,
}

impl<T> RwLock<T> {
    #[cfg(not(loom))]
    pub const fn new(v: T) -> RwLock<T> {
        RwLock {
            state: AtomicUsize::new(0),
            writer_wake_counter: AtomicUsize::new(0),
            data: UnsafeCell::new(v),
        }
    }

    #[cfg(loom)]
    pub fn new(v: T) -> RwLock<T> {
        RwLock {
            state: AtomicUsize::new(0),
            writer_wake_counter: AtomicUsize::new(0),
            data: UnsafeCell::new(v),
        }
    }

    /// acquire reader lock
    pub fn read(&self) -> RwLockReadGuard<T> {
        let mut s = self.state.load(Ordering::Relaxed);
        loop {
            if s & 1 == 0 {
                match self.state.compare_exchange_weak(
                    s,
                    s + 2,
                    Ordering::Acquire,
                    Ordering::Relaxed,
                ) {
                    Ok(_) => {
                        return RwLockReadGuard {
                            rwlock: self,
                            _phantom: Default::default(),
                        }
                    }
                    Err(e) => s = e,
                }
            }

            if s & 1 == 1 {
                while self.state.load(Ordering::Relaxed) == s {
                    hint::spin_loop();

                    #[cfg(loom)]
                    loom::thread::yield_now();
                }
                s = self.state.load(Ordering::Relaxed);
            }

            #[cfg(loom)]
            loom::thread::yield_now();
        }
    }

    /// acquire writer lock
    pub fn write(&self) -> RwLockWriteGuard<T> {
        let mut s = self.state.load(Ordering::Relaxed);
        loop {
            if s <= 1 {
                match self.state.compare_exchange(
                    s,
                    usize::MAX,
                    Ordering::Acquire,
                    Ordering::Relaxed,
                ) {
                    Ok(_) => {
                        return RwLockWriteGuard {
                            rwlock: self,
                            _phantom: PhantomData,
                        }
                    }
                    Err(e) => {
                        s = e;
                        continue;
                    }
                }
            }

            if s & 1 == 0 {
                match self
                    .state
                    .compare_exchange(s, s + 1, Ordering::Relaxed, Ordering::Relaxed)
                {
                    Ok(_) => (),
                    Err(e) => {
                        s = e;
                        continue;
                    }
                }
            }

            let w = self.writer_wake_counter.load(Ordering::Acquire);
            s = self.state.load(Ordering::Relaxed);

            if s >= 2 {
                while self.writer_wake_counter.load(Ordering::Acquire) == w {
                    hint::spin_loop();

                    #[cfg(loom)]
                    loom::thread::yield_now();
                }
                s = self.state.load(Ordering::Relaxed);
            }

            #[cfg(loom)]
            loom::thread::yield_now();
        }
    }
}

pub struct RwLockReadGuard<'a, T> {
    rwlock: &'a RwLock<T>,
    _phantom: PhantomData<*mut ()>,
}

impl<'a, T> RwLockReadGuard<'a, T> {
    /// unlock read lock
    pub fn unlock(self) {}

    #[cfg(loom)]
    pub fn with<F, R>(&self, f: F) -> R
    where
        F: FnOnce(*const T) -> R,
    {
        self.rwlock.data.with(f)
    }
}

pub struct RwLockWriteGuard<'a, T> {
    rwlock: &'a RwLock<T>,
    _phantom: PhantomData<*mut ()>,
}

impl<'a, T> RwLockWriteGuard<'a, T> {
    /// unlock write lock
    pub fn unlock(self) {}

    #[cfg(loom)]
    pub fn with_mut<F, R>(&mut self, f: F) -> R
    where
        F: FnOnce(*mut T) -> R,
    {
        self.rwlock.data.with_mut(f)
    }
}

unsafe impl<T> Sync for RwLock<T> {}
unsafe impl<T> Send for RwLock<T> {}

#[cfg(not(loom))]
impl<'a, T> Deref for RwLockReadGuard<'a, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        unsafe { &*self.rwlock.data.get() }
    }
}

#[cfg(not(loom))]
impl<'a, T> Deref for RwLockWriteGuard<'a, T> {
    type Target = T;

    fn deref(&self) -> &Self::Target {
        unsafe { &*self.rwlock.data.get() }
    }
}

#[cfg(not(loom))]
impl<'a, T> DerefMut for RwLockWriteGuard<'a, T> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        unsafe { &mut *self.rwlock.data.get() }
    }
}

/// release read lock
impl<'a, T> Drop for RwLockReadGuard<'a, T> {
    fn drop(&mut self) {
        if self.rwlock.state.fetch_sub(2, Ordering::Release) == 3 {
            self.rwlock
                .writer_wake_counter
                .fetch_add(1, Ordering::Release);
        }
    }
}

/// release write lock
impl<'a, T> Drop for RwLockWriteGuard<'a, T> {
    fn drop(&mut self) {
        self.rwlock.state.store(0, Ordering::Release);
        self.rwlock
            .writer_wake_counter
            .fetch_add(1, Ordering::Release);
    }
}

impl<T: Default> Default for RwLock<T> {
    /// Creates a new `RwLock<T>`, with the `Default` value for T.
    fn default() -> RwLock<T> {
        RwLock::new(Default::default())
    }
}