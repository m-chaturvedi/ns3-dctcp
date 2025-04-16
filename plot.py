import matplotlib.pyplot as plt
from scipy.ndimage import label
import itertools

# https://stackoverflow.com/questions/20618804/how-to-smooth-a-curve-for-a-dataset
from scipy.signal import savgol_filter
import pandas
import numpy as np


def scatter_hist(x, y, ax, ax_histx, ax_histy):
    # no labels
    ax_histx.tick_params(axis="x", labelbottom=False)
    ax_histy.tick_params(axis="y", labelleft=False)

    # the scatter plot:
    ax.scatter(x, y)

    # now determine nice limits by hand:
    binwidth = 0.25
    xymax = max(np.max(np.abs(x)), np.max(np.abs(y)))
    lim = (int(xymax / binwidth) + 1) * binwidth

    bins = np.arange(-lim, lim + binwidth, binwidth)
    ax_histx.hist(x, bins=bins)
    ax_histy.hist(y, bins=bins, orientation="horizontal")


def plot_throughput():
    df = pandas.read_csv(
        "tcp-validation-first-tcp-throughput.dat",
        delimiter=",",
        names=["seconds", "throughput"],
    )
    smooth_throughput = savgol_filter(df.throughput, 51, 3)
    plt.plot(df.seconds, smooth_throughput)

    df = pandas.read_csv(
        "tcp-validation-second-tcp-throughput.dat",
        delimiter=" ",
        names=["seconds", "throughput"],
    )
    smooth_throughput = savgol_filter(df.throughput, 51, 3)
    plt.plot(df.seconds, smooth_throughput)
    plt.show()


def plot_tcp_dctcp_queue_size_fig_1():
    file_path_tcp = "fig_1_tcp_result.dat"
    file_path_dctcp = "fig_1_dctcp_result.dat"

    df_tcp = pandas.read_csv(
        file_path_tcp,
        delimiter=" ",
        header=0,
        names=["seconds", "queue_length", "backlog"],
    )

    df_dctcp = pandas.read_csv(
        file_path_dctcp,
        delimiter=" ",
        header=0,
        names=["seconds", "queue_length", "backlog"],
    )

    y_val = df_tcp.queue_length
    plt.plot(df_tcp.seconds, y_val, label="tcp")
    plt.plot(df_dctcp.seconds, df_dctcp.queue_length, color="red", label="dctcp")
    plt.xlabel("Time (seconds)")
    plt.ylabel("Queue Length in packets")
    plt.title("Figure 1, DCTCP vs TCP queue length")
    plt.legend(loc="center left")
    plt.savefig("figure_1_tcp_dctcp_queue_size.png")
    plt.clf()


def plot_tcp_dctcp_queue_size_fig_13():
    file_path_tcp = "fig_1_tcp_result.dat"
    file_path_dctcp = "fig_1_dctcp_result.dat"
    ind = 0
    # https://github.com/axismaps/colorbrewer/
    colors = ["#d7191c", "#fdae61", "#abdda4", "#2b83ba"]

    for proto, num_flows in itertools.product(["dctcp", "tcp"], [2, 20]):
        file_name = f"figure_13_flows_{num_flows}_{proto}.dat"
        df = pandas.read_csv(
            file_name,
            delimiter=" ",
            header=0,
            names=["queue_length", "cdf"],
        )
        plt.plot(
            df.queue_length, df.cdf, color=colors[ind], label=f"{proto}_flows_{num_flows}"
        )
        ind += 1

    plt.xlabel("Queue Length (packets)")
    plt.ylabel("CDF")
    plt.title("Figure 13, DCTCP vs TCP cdf")
    plt.legend(loc="best")
    plt.savefig("figure_13_tcp_dctcp_cdf.png")


def main():
    #  plot_throughput()
    plot_tcp_dctcp_queue_size_fig_1()
    plot_tcp_dctcp_queue_size_fig_13()


if __name__ == "__main__":
    main()
