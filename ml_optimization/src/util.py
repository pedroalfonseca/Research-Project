from time import time
import tracemalloc

import numpy as np
from scipy.special import rel_entr
import pandas as pd

from matplotlib import pyplot as plt
import seaborn as sns

import optuna


# ---- Benchmarking ------------------------------------------------------------

def benchmark(func):
    def wrapper(*args, **kwargs):
        tracemalloc.start()
        snapshot1 = tracemalloc.take_snapshot()

        time1 = time()

        result = func(*args, **kwargs)

        time2 = time()

        snapshot2 = tracemalloc.take_snapshot()
        tracemalloc.stop()

        exe_time = time2 - time1
        stats = snapshot2.compare_to(snapshot1, "lineno")
        mem_usage = sum(stat.size_diff for stat in stats)

        return result, exe_time, mem_usage

    return wrapper


# ---- Math --------------------------------------------------------------------

def kl_divergence(p, q):
    return np.sum(rel_entr(p, q))


def symmetrized_kl_divergence(p, q):
    return kl_divergence(p, q) + kl_divergence(q, p)


def compute_relative_errors(Y_val, Y_val_pred):
    with np.errstate(divide="ignore", invalid="ignore"):
        relative_errors = np.abs((Y_val - Y_val_pred) / Y_val)
        relative_errors[Y_val == 0] = np.nan
    return relative_errors


# ---- Machine Learning --------------------------------------------------------

@benchmark
def create_and_train_model(X, Y, ModelClass, *args, **kwargs):
    model = ModelClass(*args, **kwargs)
    model.fit(X, Y)

    return model


@benchmark
def test_model(X, model):
    Y_pred = model.predict(X)

    return Y_pred


# ---- Plotting ----------------------------------------------------------------

def plot_series(ax, df, x_col, y_col, series_index=0):
    palette = list(sns.palettes.mpl_palette("Dark2"))
    xs = df[x_col]
    ys = df[y_col]
    building_id = df["building_id"].iloc[0]

    color = palette[series_index % len(palette)]
    
    ax.plot(xs, ys, label=f"Building {building_id}", color=color)


def plot_predicted_vs_actual(Y_val, Y_pred, col_names):
    plt.figure(figsize=(15, 10))

    for i, col_name in enumerate(col_names):
        plt.subplot(3, 3, i + 1)
        plt.scatter(Y_val.iloc[:, i], Y_pred[:, i], alpha=0.5)
        plt.plot([Y_val.iloc[:, i].min(), Y_val.iloc[:, i].max()],
                 [Y_val.iloc[:, i].min(), Y_val.iloc[:, i].max()], "r--")
        plt.xlabel("Actual")
        plt.ylabel("Predicted")
        plt.title(f"Predicted vs Actual '{col_name}'")
        plt.grid(True)

    plt.tight_layout()
    plt.show()


def plot_relative_errors(Y_val, Y_pred, col_names):
    relative_errors = compute_relative_errors(Y_val, Y_pred)

    plt.figure(figsize=(15, 10))

    for i, col_name in enumerate(col_names):
        plt.subplot(3, 3, i + 1)
        sns.histplot(relative_errors.values[:, i], bins=30, kde=True)
        plt.xlabel("Relative Error")
        plt.ylabel("Frequency")
        plt.title(f"Relative Errors for '{col_name}'")
        plt.grid(True)

    plt.tight_layout()
    plt.show()

    mean_relative_errors = np.nanmean(relative_errors, axis=0)
    max_col_name_len = max(len(col_name) for col_name in col_names)

    for i, mean_error in enumerate(mean_relative_errors):
        col_name = col_names[i]
        print(f"Mean Relative Error for '{col_name}': {' ' * (max_col_name_len - len(col_name))}{mean_error:.4f}")


def plot_cumulative_error_distribution(Y_val, Y_pred, col_names):
    relative_errors = compute_relative_errors(Y_val, Y_pred)

    plt.figure(figsize=(15, 10))

    # Plot cumulative error distributions
    for i, col_name in enumerate(col_names):
        plt.subplot(3, 3, i + 1)

        # Compute relative errors for color predictions
        errors = relative_errors.values[:, i]
        errors_sorted = np.sort(errors[~np.isnan(errors)])  # Remove NaNs and sort
        cumulative_percent = np.arange(1, len(errors_sorted) + 1) / len(errors_sorted) * 100

        plt.plot(errors_sorted, cumulative_percent, color="crimson")
        plt.xlabel("Relative Error")
        plt.ylabel("Cumulative Percentage")
        plt.title(f"Cumulative Error Distribution for '{col_name}'")
        plt.grid(True)

    plt.tight_layout()
    plt.show()


def plot_hyperparameter_distributions(study):
    trials = study.trials
    params = [trial.params for trial in trials if trial.state == optuna.trial.TrialState.COMPLETE]
    df = pd.DataFrame(params).select_dtypes(include=[np.number])

    # Plot box plots for each hyperparameter
    plt.figure(figsize=(12, 8))
    sns.boxplot(data=df)
    plt.title("Hyperparameter distributions")
    plt.xticks(rotation=45)
    plt.show()

    # Compute confidence intervals
    for i, col_name in enumerate(df.columns):
        mean = np.mean(df[col_name])
        std = np.std(df[col_name])
        ci = (mean - 1.96 * std, mean + 1.96 * std)  # 95% CI

        print(f"{"\n" if i != 0 else ""}-- {col_name} --")
        print(f"Mean: {mean:.4f}")
        print(f"CI:   [{ci[0]:.4f}, {ci[1]:.4f}]")
