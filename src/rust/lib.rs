mod hb;
#[allow(unused_imports)]
use hb::*;

#[cfg(feature = "font")]
mod font;
#[cfg(feature = "shape")]
mod shape;

#[cfg(any(feature = "font", feature = "shape"))]
struct HbBlob(*mut hb_blob_t);

#[cfg(any(feature = "font", feature = "shape"))]
impl Drop for HbBlob {
    fn drop(&mut self) {
        unsafe {
            hb_blob_destroy(self.0);
        }
    }
}

#[cfg(any(feature = "font", feature = "shape"))]
impl AsRef<[u8]> for HbBlob {
    fn as_ref(&self) -> &[u8] {
        let mut length = 0;
        let data = unsafe { hb_blob_get_data(self.0, &mut length) };
        if data.is_null() {
            &[]
        } else {
            unsafe { std::slice::from_raw_parts(data.cast(), length as usize) }
        }
    }
}

#[cfg(any(feature = "font", feature = "shape"))]
unsafe impl Send for HbBlob {}
#[cfg(any(feature = "font", feature = "shape"))]
unsafe impl Sync for HbBlob {}

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
