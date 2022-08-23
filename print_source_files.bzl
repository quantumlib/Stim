def print_source_files_impl(ctx):
    source_files_no_main_out = ctx.actions.declare_file("source_files_no_main.out")
    test_files_out = ctx.actions.declare_file("test_files.out")
    perf_files_out = ctx.actions.declare_file("perf_files.out")
    pybind_files_out = ctx.actions.declare_file("pybind_files.out")

    ctx.actions.write(
        output = source_files_no_main_out,
        content = "\n".join(ctx.attr.source_files_no_main),
    )

    ctx.actions.write(
        output = test_files_out,
        content = "\n".join(ctx.attr.test_files),
    )

    ctx.actions.write(
        output = perf_files_out,
        content = "\n".join(ctx.attr.perf_files),
    )

    ctx.actions.write(
        output = pybind_files_out,
        content = "\n".join(ctx.attr.pybind_files),
    )

    return [DefaultInfo(files = depset([
        source_files_no_main_out,
        test_files_out,
        perf_files_out,
        pybind_files_out
    ]))]

print_source_files = rule(
    implementation = print_source_files_impl,
    attrs = {
        "source_files_no_main" : attr.string_list(),
        "test_files" : attr.string_list(),
        "perf_files" : attr.string_list(),
        "pybind_files" : attr.string_list(),
    },
)
