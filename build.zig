const std = @import("std");
const Builder = std.build.Builder;

// fn addStaticLibrary(b: *Builder, name: []const u8, root_src: ?[]const u8, target: std.zig.CrossTarget, mode: std.builtin.Mode) *std.build.LibExeObjStep {
//     const lib = b.addStaticLibrary(name, root_src);
//     lib.setTarget(target);
//     lib.setBuildMode(mode);
//     return lib;
// }

//     const digits_lib = addStaticLibrary(b, "digits_png", "digits.zig", target, mode);
//     const stb_image_lib = addStaticLibrary(b, "stb_image_impl", "stb_image_impl.c", target, mode);

pub fn build(b: *Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const digits_lib = b.addStaticLibrary("digits_png", "digits.zig");
    digits_lib.setTarget(target);
    digits_lib.setBuildMode(mode);
    const stb_image_lib = b.addStaticLibrary("stb_image_impl", "stb_image_impl.c");
    stb_image_lib.linkLibC();
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
