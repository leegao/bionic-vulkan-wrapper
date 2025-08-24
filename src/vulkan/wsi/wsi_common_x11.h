#pragma once

struct wsi_x11_connection {
   bool has_dri3;
   bool has_dri3_modifiers;
   bool has_present;
   bool has_mit_shm;
};

struct wsi_x11 {
   struct wsi_interface base;

   pthread_mutex_t                              mutex;
   /* Hash table of xcb_connection -> wsi_x11_connection mappings */
   struct hash_table *connections;
};

struct x11_image {
   struct wsi_image                          base;
   xcb_pixmap_t                              pixmap;
   atomic_bool                               busy;
   bool                                      present_queued;
   uint32_t                                  sync_fence;
   uint32_t                                  serial;
   xcb_shm_seg_t                             shmseg;
   int                                       shmid;
   uint8_t *                                 shmaddr;
   uint64_t                                  present_id;
   uint64_t                                  signal_present_id;
};

struct x11_swapchain {
   struct wsi_swapchain                        base;

   bool                                         has_dri3_modifiers;
   bool                                         has_mit_shm;

   xcb_connection_t *                           conn;
   xcb_window_t                                 window;
   xcb_gc_t                                     gc;
   uint32_t                                     depth;
   VkExtent2D                                   extent;

   xcb_present_event_t                          event_id;
   xcb_special_event_t *                        special_event;
   uint64_t                                     send_sbc;
   uint64_t                                     last_present_msc;
   uint32_t                                     stamp;
   atomic_int                                   sent_image_count;

   bool                                         has_present_queue;
   bool                                         has_acquire_queue;
   VkResult                                     status;
   bool                                         copy_is_suboptimal;
   struct wsi_queue                             present_queue;
   struct wsi_queue                             acquire_queue;
   pthread_t                                    queue_manager;

   /* Lock and condition variable that lets callers monitor forward progress in the swapchain.
    * This includes:
    * - Present ID completion updates (present_id).
    * - Pending ID pending updates (present_id_pending).
    * - Any errors happening while blocking on present progress updates (present_progress_error).
    * - present_submitted_count.
    */
   pthread_mutex_t                              present_progress_mutex;
   pthread_cond_t                               present_progress_cond;

   /* Lock needs to be taken when waiting for and reading presentation events.
    * Only relevant in non-FIFO modes where AcquireNextImage or WaitForPresentKHR may
    * have to pump the XCB connection on its own. */
   pthread_mutex_t                              present_poll_mutex;

   /* For VK_KHR_present_wait. */
   uint64_t                                     present_id;
   uint64_t                                     present_id_pending;

   /* When blocking on present progress, this can be set and progress_cond is signalled to unblock waiters. */
   VkResult                                     present_progress_error;

   /* For handling wait_ready scenario where two different threads can pump the connection. */

   /* Updated by presentation thread. Incremented when a present is submitted to X.
    * Signals progress_cond when this happens. */
   uint64_t                                     present_submitted_count;
   /* Total number of images ever pushed to a present queue. */
   uint64_t                                     present_queue_push_count;
   /* Total number of images returned to application in AcquireNextImage. */
   uint64_t                                     present_poll_acquire_count;

   struct x11_image                             images[0];
};
VK_DEFINE_NONDISP_HANDLE_CASTS(x11_swapchain, base.base, VkSwapchainKHR,
                               VK_OBJECT_TYPE_SWAPCHAIN_KHR)