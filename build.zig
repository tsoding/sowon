const Builder = @import("std").build.Builder;

pub fn build(b: *Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const digits_lib = b.addStaticLibrary("digits_png", "digits.zig");
    const exe = b.addExecutable("sowon", "main.c");
    exe.setTarget(target);
    exe.setBuildMode(mode);
    exe.step.dependOn(&digits_lib.step);
    exe.linkLibC();
    exe.linkSystemLibrary("sdl2");
    exe.linkSystemLibrary("m");
    exe.linkLibrary(digits_lib);
    exe.addIncludeDir(".");
    exe.defineCMacro("STB_IMAGE_IMPLEMENTATION");
    exe.install();

    const run_cmd = exe.run();
    run_cmd.step.dependOn(b.getInstallStep());
    if (b.args) |args| {
        run_cmd.addArgs(args);
    }

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
