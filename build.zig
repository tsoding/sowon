const std = @import("std");
const Builder = std.build.Builder;

pub fn build(b: *Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const digits_lib = b.addStaticLibrary("digits_png", "digits.zig");
    digits_lib.setTarget(target);
    digits_lib.setBuildMode(mode);

    const stb_image_lib = b.addStaticLibrary("stb_image_impl", "stb_image_impl.c");
    stb_image_lib.linkLibC();
    // stb_image_lib.setTarget(target);
    // stb_image_lib.setBuildMode(mode);
    // adding these 2 lines causes windows ci to fail... not sure why
    // error message is:
    // Run zig build -Dtarget=x86_64-windows-msvc
    // error(compilation): clang failed with stderr: In file included from D:\a\sowon\sowon\stb_image_impl.c:2:
    // D:\a\sowon\sowon/./stb_image.h:324:10: fatal error: 'stdio.h' file not found

    const exe = b.addExecutable("sowon", "main.c");
    exe.setTarget(target);
    exe.setBuildMode(mode);
    exe.step.dependOn(&digits_lib.step);
    exe.step.dependOn(&stb_image_lib.step);
    exe.linkLibC();
    exe.linkSystemLibrary("sdl2");
    exe.linkSystemLibrary("m");
    exe.linkLibrary(digits_lib);
    exe.linkLibrary(stb_image_lib);

    // windows specific
    if (target.os_tag) |os| {
        if (os == .windows) {
            // set INCLUDES=/I SDL2\include
            const sdl_inc_path = std.fs.path.joinWindows(b.allocator, &[_][]const u8{ "SDL2", "include" }) catch |e| {
                std.debug.print("unable to join path \n", .{});
                return;
            };
            exe.addIncludeDir(sdl_inc_path);
            // set LIBS=SDL2\lib\x64\SDL2.lib SDL2\lib\x64\SDL2main.lib Shell32.lib
            const sdl_lib_path = std.fs.path.joinWindows(b.allocator, &[_][]const u8{ "SDL2", "lib", "x64" }) catch |e| {
                std.debug.print("unable to join path \n", .{});
                return;
            };
            exe.addLibPath(sdl_lib_path);
            exe.linkSystemLibrary("SDL2main");
            exe.linkSystemLibrary("Shell32");
            exe.subsystem = .Windows;

            // workaround for missing includes: https://github.com/ziglang/zig/issues/5402
            if (b.env_map.get("INCLUDE")) |entry| {
                var it = std.mem.split(entry, ";");
                while (it.next()) |path| {
                    exe.addIncludeDir(path);
                }
            }
        }
    }
    exe.install();

    const run_cmd = exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
