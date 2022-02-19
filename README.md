# kMemInspect

A device for reading kernel memory.

In order to specify an address to read from, write it to the device (for example, write `"ffeeddccbbaa9988"` to `/dev/kmem_inspect`).
In order to read from this address, read from the device (`/dev/kmem_inspect`). The module parameter `content_len` is used to determine the amount of bytes of the address content.
