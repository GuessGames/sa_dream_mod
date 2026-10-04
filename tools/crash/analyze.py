"""Symbolizes SA Dream Mod crash reports (client CoopAndreas_crashes/*.log and server_*.log).

    python tools/crash/analyze.py <report.log> [more reports...]

* CoopAndreasSA.dll / server.exe addresses -> function + file:line, using the PDB archived for that release
  in <dev>/symbols/<release>/ (written by the manager on publish), or the current build as a fallback.
* gta_sa.exe addresses -> the nearest game function known to plugin-sdk (its "Converted from ... 0xADDR" comments).
Prints the exception, the symbolized backtrace and the last log lines; the crash watcher reads this output.
"""
import ctypes
import ctypes.wintypes as wt
import os
import re
import struct
import sys
from bisect import bisect_right

SRC = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SYMBOLS = os.path.join(os.path.dirname(SRC), "symbols")
BUILD = os.path.join(SRC, "build", "windows", "x86", "release")
PLUGIN_SDK = os.path.join(SRC, "third_party", "plugin-sdk", "plugin_sa", "game_sa")
FAKE_BASE = 0x10000000


# ------------------------------------------------------------------ dbghelp
class SYMBOL_INFO(ctypes.Structure):
    _fields_ = [("SizeOfStruct", wt.ULONG), ("TypeIndex", wt.ULONG), ("Reserved", ctypes.c_uint64 * 2), ("Index", wt.ULONG),
                ("Size", wt.ULONG), ("ModBase", ctypes.c_uint64), ("Flags", wt.ULONG), ("Value", ctypes.c_uint64),
                ("Address", ctypes.c_uint64), ("Register", wt.ULONG), ("Scope", wt.ULONG), ("Tag", wt.ULONG),
                ("NameLen", wt.ULONG), ("MaxNameLen", wt.ULONG), ("Name", ctypes.c_char * 512)]


class IMAGEHLP_LINE64(ctypes.Structure):
    _fields_ = [("SizeOfStruct", wt.DWORD), ("Key", ctypes.c_void_p), ("LineNumber", wt.DWORD), ("FileName", ctypes.c_char_p),
                ("Address", ctypes.c_uint64)]


class Symbolizer:
    def __init__(self):
        self.dbg = ctypes.WinDLL("dbghelp.dll")
        self.dbg.SymLoadModuleEx.restype = ctypes.c_uint64
        self.dbg.SymLoadModuleEx.argtypes = [wt.HANDLE, wt.HANDLE, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_uint64, wt.DWORD,
                                             ctypes.c_void_p, wt.DWORD]
        self.dbg.SymFromAddr.argtypes = [wt.HANDLE, ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint64), ctypes.POINTER(SYMBOL_INFO)]
        self.dbg.SymGetLineFromAddr64.argtypes = [wt.HANDLE, ctypes.c_uint64, ctypes.POINTER(wt.DWORD), ctypes.POINTER(IMAGEHLP_LINE64)]
        self.h = wt.HANDLE(0x5AD5)  # any unique value works as a "process" handle
        self.dbg.SymSetOptions(0x2 | 0x10 | 0x200)  # UNDNAME | LOAD_LINES | FAIL_CRITICAL_ERRORS
        self.dbg.SymInitialize(self.h, None, False)
        self.modules = {}  # name -> base

    def load(self, name, image):
        if name in self.modules:
            return self.modules[name]
        base = FAKE_BASE + 0x2000000 * len(self.modules)
        self.dbg.SymSetSearchPath(self.h, os.path.dirname(image).encode())
        got = self.dbg.SymLoadModuleEx(self.h, None, image.encode(), None, base, 0, None, 0)
        self.modules[name] = got or None
        return self.modules[name]

    def resolve(self, name, rva):
        base = self.modules.get(name)
        if not base:
            return None
        sym = SYMBOL_INFO()
        sym.SizeOfStruct = 88  # sizeof(SYMBOL_INFO) with Name[1] on x64
        sym.MaxNameLen = 511
        disp = ctypes.c_uint64()
        if not self.dbg.SymFromAddr(self.h, base + rva, ctypes.byref(disp), ctypes.byref(sym)):
            return None
        text = "%s+0x%x" % (sym.Name.decode(errors="replace"), disp.value)
        line = IMAGEHLP_LINE64()
        line.SizeOfStruct = ctypes.sizeof(IMAGEHLP_LINE64)
        d32 = wt.DWORD()
        if self.dbg.SymGetLineFromAddr64(self.h, base + rva, ctypes.byref(d32), ctypes.byref(line)):
            path = line.FileName.decode(errors="replace")
            if path.lower().startswith(SRC.lower()):
                path = os.path.relpath(path, SRC)
            text += "  (%s:%d)" % (path, line.LineNumber)
        return text


# ------------------------------------------------------------------ game functions known to plugin-sdk
_game = None


def game_functions():
    global _game
    if _game is None:
        found = {}
        pattern = re.compile(r"(?:Converted from|//)\s*(.*?)\s*(0x[0-9A-Fa-f]{6,7})\b")
        for root, _, files in os.walk(PLUGIN_SDK):
            for f in files:
                if not f.endswith(".cpp"):
                    continue
                for line in open(os.path.join(root, f), encoding="utf-8", errors="replace"):
                    m = pattern.search(line)
                    if m and "(" in m.group(1):
                        addr = int(m.group(2), 16)
                        if 0x401000 <= addr < 0x900000:
                            found.setdefault(addr, re.sub(r"^\w+\s+", "", m.group(1))[:120])
        addrs = sorted(found)
        _game = (addrs, [found[a] for a in addrs])
    return _game


def game_name(addr):
    addrs, names = game_functions()
    i = bisect_right(addrs, addr) - 1
    if i < 0 or addr - addrs[i] > 0x2000:
        return None
    return "%s +0x%x" % (names[i], addr - addrs[i])


# ------------------------------------------------------------------ minidump reading (no debugger needed)
def read_minidump(path):
    """(esp, eip, [(start, bytes)], {module name: (base, size, path)}) of the exception thread, or None"""
    d = open(path, "rb").read()
    if d[:4] != b"MDMP":
        return None
    count, rva = struct.unpack_from("<II", d, 8)
    streams = {}
    for i in range(count):
        kind, size, srva = struct.unpack_from("<III", d, rva + i * 12)
        streams[kind] = (size, srva)
    if 6 not in streams:
        return None
    erva = streams[6][1]
    ctx_rva = struct.unpack_from("<II", d, erva + 8 + 152)[1]  # MINIDUMP_EXCEPTION_STREAM.ThreadContext
    eip = struct.unpack_from("<I", d, ctx_rva + 0xB8)[0]          # x86 CONTEXT
    esp = struct.unpack_from("<I", d, ctx_rva + 0xC4)[0]
    memory = []
    if 5 in streams:
        mrva = streams[5][1]
        for i in range(struct.unpack_from("<I", d, mrva)[0]):
            start, size, drva = struct.unpack_from("<QII", d, mrva + 4 + i * 16)
            memory.append((start, d[drva:drva + size]))
    if 9 in streams:  # full dumps ("Save Dump" in the crash dialog): Memory64ListStream, data stored back to back
        mrva = streams[9][1]
        n, data_rva = struct.unpack_from("<QQ", d, mrva)
        for i in range(n):
            start, size = struct.unpack_from("<QQ", d, mrva + 16 + i * 16)
            memory.append((start, d[data_rva:data_rva + size]))
            data_rva += size
    modules = {}
    if 4 in streams:
        lrva = streams[4][1]
        for i in range(struct.unpack_from("<I", d, lrva)[0]):
            m = lrva + 4 + i * 108
            base, size = struct.unpack_from("<QI", d, m)
            name_rva = struct.unpack_from("<I", d, m + 20)[0]
            n = struct.unpack_from("<I", d, name_rva)[0]
            full = d[name_rva + 4:name_rva + 4 + n].decode("utf-16-le", errors="replace")
            modules[os.path.basename(full).lower()] = (base, size, full)
    return esp, eip, memory, modules


class CodeReader:
    """Code bytes of gta_sa.exe and our dll (from disk) to check that an address follows a CALL."""

    def __init__(self, modules, sdir):
        self.mods = {}
        for name in ("gta_sa.exe", "coopandreassa.dll", "server.exe"):
            if name not in modules:
                continue
            base, size, full = modules[name]
            image = full if name == "gta_sa.exe" else os.path.join(sdir, os.path.basename(full))
            if not os.path.exists(image):
                image = full
            if os.path.exists(image):
                self.mods[name] = (base, size, self._map(image))

    @staticmethod
    def _map(image):
        d = open(image, "rb").read()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        opt = struct.unpack_from("<H", d, pe + 20)[0]
        sections = []
        o = pe + 24 + opt
        for _ in range(nsec):
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", d, o + 8)
            sections.append((va, max(vsize, rsize), rptr, rsize))
            o += 40
        return d, sections

    def module_of(self, v):
        for name, (base, size, _) in self.mods.items():
            if base <= v < base + size:
                return name, base
        return None

    def _bytes(self, v, before):
        name, base = self.module_of(v)
        d, sections = self.mods[name][2]
        rva = v - base - before
        for va, vs, rptr, rsize in sections:
            if va <= rva and rva + before <= va + min(vs, rsize):
                return d[rptr + rva - va:rptr + rva - va + before]
        return None

    def follows_call(self, v):
        b = self._bytes(v, 7)
        if not b:
            return False
        if b[2] == 0xE8:                                   # call rel32
            return True
        for at in (5, 4, 1, 0):                            # call r/m32: FF /2 with mod/disp of 1..6 bytes
            if b[at] == 0xFF and (b[at + 1] >> 3) & 7 == 2:
                return True
        return False


# ------------------------------------------------------------------ report parsing
def symbols_dir(release):
    if release:
        d = os.path.join(SYMBOLS, release)
        if os.path.isdir(d):
            return d, "release " + release
    return BUILD, "CURRENT BUILD (no archived symbols for release %r - lines may be off)" % release


def analyze(path):
    text = open(path, encoding="utf-8", errors="replace").read().replace("\r", "")
    lines = text.split("\n")
    out = ["=" * 100, "report: " + path]

    release = None
    m = re.search(r"SA Dream Mod release: ([^,\s]+)", text)
    if m and m.group(1) != "unknown":
        release = m.group(1)
    sdir, how = symbols_dir(release)
    out.append("symbols: %s  [%s]" % (sdir, how))

    sym = Symbolizer()
    for name, pdb_image in (("coopandreassa.dll", "CoopAndreasSA.dll"), ("server.exe", "server.exe")):
        image = os.path.join(sdir, pdb_image)
        if os.path.exists(image):
            sym.load(name, image)

    # module bases from the client's "Loaded modules" list
    bases = {}
    for b, mod in re.findall(r"Base: 0x([0-9A-Fa-f]+)\s+Module: ([^\n]+?)\s+Version:", text):
        bases[os.path.basename(mod).lower()] = int(b, 16)

    def describe(module, rva, absolute=None):
        module = module.lower()
        if module == "gta_sa.exe":
            addr = absolute if absolute is not None else 0x400000 + rva
            return "gta_sa.exe 0x%06X  %s" % (addr, game_name(addr) or "?")
        r = sym.resolve(module, rva)
        return "%s +0x%X  %s" % (module, rva, r or "?")

    for l in lines:
        if l.startswith("Unhandled exception") or l.startswith("0x") or l.startswith("Access violation") or "SA Dream Mod" in l:
            out.append(l.strip())
    m = re.search(r"Unhandled exception (?:0x[0-9A-F]+ )?at 0x([0-9A-Fa-f]+) in (\S+) \(\+0x([0-9A-Fa-f]+)\)", text)
    if m:
        out.append("CRASH AT: " + describe(m.group(2), int(m.group(3), 16), int(m.group(1), 16)))

    out.append("backtrace:")
    in_trace = False
    for l in lines:
        if l.startswith("Backtrace:"):
            in_trace = True
            continue
        if in_trace:
            if not l.strip():
                break
            # client: "   0x<sym>: name in module (+0x..) (0x<pc>)"   server: "   0x<pc> in module (+0x<rva>)"
            mc = re.match(r"\s+0x[0-9A-Fa-f]+: .*? in (\S+) \(\+0x[0-9a-fA-F]+\)(?: \(0x([0-9A-Fa-f]+)\))?", l)
            ms = re.match(r"\s+0x([0-9A-Fa-f]+) in (\S+) \(\+0x([0-9A-Fa-f]+)\)", l)
            if ms:
                out.append("   " + describe(ms.group(2), int(ms.group(3), 16), int(ms.group(1), 16)))
            elif mc and mc.group(2):
                module, pc = mc.group(1), int(mc.group(2), 16)
                base = bases.get(module.lower(), 0x400000 if module.lower() == "gta_sa.exe" else None)
                out.append("   " + (describe(module, pc - base, pc) if base is not None else l.strip()))
            else:
                out.append("   " + l.strip())

    # the minidump next to the report has the whole stack of the crashed thread: every value that points right
    # behind a CALL instruction is a real return address (much better than the frame walk, which stops early)
    dump = os.path.splitext(path)[0] + ".dmp"
    md = read_minidump(dump) if os.path.exists(dump) and os.path.getsize(dump) < 2048 * 1024 * 1024 else None
    if md:
        esp, eip, memory, modules = md
        # reports from before the fix walked the stack in the context itself: the report's ESP is the real one
        m_esp = re.search(r"ESP: 0x([0-9A-Fa-f]{8})", text)
        if m_esp:
            esp = int(m_esp.group(1), 16)
        code = CodeReader(modules, sdir)
        stack = next((data for start, data in memory if start <= esp < start + len(data)), None)
        out.append("stack from the minidump (return addresses behind a CALL, newest first):")
        if stack:
            start = next(s for s, data in memory if data is stack)
            count = 0
            for off in range(esp - start, len(stack) - 3, 4):
                v = struct.unpack_from("<I", stack, off)[0]
                mod = code.module_of(v)
                if not mod or not code.follows_call(v):
                    continue
                name, base = mod
                out.append("   [esp+0x%04X] %s" % (start + off - esp, describe(name, v - base, v)))
                count += 1
                if count >= 40:
                    break
    # frame-pointer walks often stop at the first frame: values on the stack that point into code are likely
    # return addresses (heuristic, oldest last)
    elif "Stack dump:" in text:
        dll_base = bases.get("coopandreassa.dll")
        seen = []
        for word in re.findall(r"([0-9A-F]{8})", text.split("Stack dump:")[1].split("base:")[0]):
            v = int(word, 16)
            if 0x401000 <= v < 0x857000 or 0x1560000 <= v < 0x1600000:
                d = describe("gta_sa.exe", v - 0x400000, v)
            elif dll_base and dll_base <= v < dll_base + 0x400000:
                d = describe("coopandreassa.dll", v - dll_base, v)
            else:
                continue
            if d not in seen:
                seen.append(d)
        out.append("stack scan (possible return addresses):")
        out.extend("   " + d for d in seen[:25])

    if "Active scripts:" in text:
        scripts = text.split("Active scripts:")[1].split("\n\n")[0].strip().split("\n")
        out.append("active scripts: " + ", ".join(s.split()[0] for s in scripts if s.strip())[:400])
    if "Last log lines:" in text:
        out.append("last log lines:")
        out.extend(text.split("Last log lines:")[1].strip("\n").split("\n")[-40:])
    return "\n".join(out)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    for p in sys.argv[1:]:
        print(analyze(p))
