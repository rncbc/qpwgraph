# How To Use The Patchbay ![Patchbay](../src/images/itemPatchbay.png) 

Once a connection patchbay established, it is possible to store those connections in a patchbay configuration that can be then restored when loaded back.

## *Activated* button ![Activated](../src/images/itemActivate.png)

The *Activated* button is simply to activate or not the loaded patchbay. The patchbay must be activated in order to access all the other functionalities.

If checked, connections stored in the loaded patchbay will be restored. All other connections will stay as they were before the load and activation of the patchbay. If and when unchecked, all previously pinned connections will be dropped.

## *Exclusive* button ![Exclusive](../src/images/itemExclusive.png)

The *Exclusive* button goal it to determine if the connections stored in the loaded and activated patchbay will be the only active connections or if previous connections are allowed.

If checked, connections not stored in the loaded and activated patchbay will be removed. If unchecked, the loaded and activated patchbay will only create the stored connections.

## *Edit* button ![Edit](../src/images/itemEdit.png)

If checked, the buttons *Pin* and *Unpin* are available. Those functions are only for connections, so at least one connection (line between an input and an output) has to be selected.

### *Pin* button ![Pin](../src/images/itemPin.png)

Makes the connection persistant in the currently loaded and activated patchbay.

### *Unpin* button ![Unpin](../src/images/itemUnpin.png)

Used to make the connection temporary. The unpinned connection will not be dropped if the current patchbay is deactivated.

## Auto Pin option

If checked, all manual connections will be pinned to the current patchbay and persistant when activated.

## Auto Disconnect option

If checked, all pinned connections will be automatically disconnected when the current patchbay is deactivated.

## Manage... dialog

Manages the currently loaded patchbay, allowing to remove and cleanup connection rules that are no longer used, obsolete or not applicable anymore.


---
Credits: @Lootre (a.k.a. Thomas Lachat).
