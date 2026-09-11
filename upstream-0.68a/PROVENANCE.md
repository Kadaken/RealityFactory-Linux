# Reality Factory 0.68A NPC/navigation recovery

These six source files were imported from RealityFactory commit
`250b75047d19bf0633fead05091c8a46462c45fe` in the upstream repository:

- `CNPC.cpp` — `44bc496baf469bbedf407ac252ad0f35b8a9db08f3d47fd4a40de6eebf7e61be`
- `CNPC.h` — `7cd5189c585a206255a7011401cc9e48ef5d8cd1f36c995b5b47a52d241fe8a2`
- `CNPCPathPoint.cpp` — `4b8b2479c0f6327b4a9186e54111400c1d336a65ee09f737f5851715b11531b8`
- `CNPCPathPoint.h` — `856a129ea3e0fcb55bdf570c0f1da1fb48e2a406e228f1cfecf8c01645a8833e`
- `Track.cpp` — `083906c898ccfd29d4ad65f7ee77c5223bc43fd4dd1c22a7954500b0cc8b9d60`
- `track.h` — `7ff16b68baa2df305222fde31b06f73af404aecc4d846eca732bdcd6faca01a3`

`NonPlayerCharacter` and its `CTrack` navigation dependency were removed from
the later Reality Factory runtime. They are retained separately from
`upstream/` so the 0.76.1 baseline and every compiler-required adaptation
remain auditable. See `../MODIFICATIONS.md` for post-import changes.

