mod hb;
#[allow(unused_imports)]
use hb::*;

#[cfg(feature = "font")]
mod font;
#[cfg(feature = "shape")]
mod shape;

#[cfg(feature = "hb-allocator")]
use std::alloc::{GlobalAlloc, Layout};
#[cfg(feature = "hb-allocator")]
use std::os::raw::c_void;

#[cfg(feature = "hb-allocator")]
struct MyAllocator;

#[cfg(feature = "hb-allocator")]
unsafe impl GlobalAlloc for MyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        assert!(layout.align() <= 2 * std::mem::size_of::<*mut u8>());
        hb_malloc(layout.size()) as *mut u8
    }

    unsafe fn alloc_zeroed(&self, layout: Layout) -> *mut u8 {
        assert!(layout.align() <= 2 * std::mem::size_of::<*mut u8>());
        hb_calloc(layout.size(), 1) as *mut u8
    }

    unsafe fn realloc(&self, ptr: *mut u8, layout: Layout, new_size: usize) -> *mut u8 {
        assert!(layout.align() <= 2 * std::mem::size_of::<*mut u8>());
        hb_realloc(ptr as *mut c_void, new_size) as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        assert!(layout.align() <= 2 * std::mem::size_of::<*mut u8>());
        hb_free(ptr as *mut c_void);
    }
}

#[cfg(feature = "hb-allocator")]
#[global_allocator]
static GLOBAL: MyAllocator = MyAllocator;
