# Source provenance

HEN: https://github.com/Scene-Collective/ps4-hen at 8c62f6628827055df0dfece1c4d14e045761288b (explicit 14.02 port).
Plugins: https://github.com/Scene-Collective/ps4-hen-plugins at 49534177d69d17609bcc09cd185c88df42fed8b5. Its original notices and third-party credits are retained under source. plugin_server includes GPL-licensed FTP/klog components; see its LICENSE.plugin_server.txt and original README.
Payload SDK: https://github.com/Scene-Collective/ps4-payload-sdk at fd4f7299d1da5f5601c164a02b6d6d019ac8630a.
libjbc: https://github.com/illusion0001/ps4-libjbc at ced297b9d1e6bee900c45d0b5a31d0669329e16d.
Toolchain: OpenOrbis v0.5.4, llvm18 release; locally compiled with Clang/LLD19 against its headers and libraries.
Host: user-supplied afterglow-goldhen-host-main archive, revision c33ab12470f9580d92eba5b51e5ad75160c3116b, attributed upstream to ArabPixel/WebKitty b4423641016f05e8782c9c80c066e5f4fbfbc551. AGPLv3 text in source/HOST_LICENSE; modified browser sources are in site/chain.
Logo: supplied by user; converted to expected menu asset dimensions.
BalenciHEN changes: menu labels, installer notices, source-build integration, raw userland payload receiver and sender, raw host loader memory preparation and dispatch error handling.


Fortnite button: original BalenciHEN local-file reset code; behavior based on the user-supplied screenshot, whose ban-removal claim has not been verified.

Home restore: removed stock panel-name mutations, offline/LiveArea return patches and forced developer-panel creation. Kept PS Plus slot replacement per user request.

Cheat menu, engine, cJSON parser, database and uploader removed in this revision. Existing generic process-info command remains required by the Fortnite local-file reset.
