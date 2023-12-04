import torch
import os
from utils.general_utils import safe_state
from argparse import ArgumentParser
from arguments import ModelParams, PipelineParams, get_combined_args
from gaussian_renderer import GaussianModel

if __name__ == "__main__":
    # Set up command line argument parser
    parser = ArgumentParser(description="Testing script parameters")
    model = ModelParams(parser, sentinel=True)
    pipeline = PipelineParams(parser)
    parser.add_argument("--output_ply", type=str, default="./output.ply")
    parser.add_argument("--quiet", action="store_true")
    args = get_combined_args(parser)
    print("create fused ply for " + args.model_path)

    # Initialize system state (RNG)
    safe_state(args.quiet)
    dataset = model.extract(args)
    gaussians = GaussianModel(dataset.sh_degree)
        
    gaussians.load_ply(os.path.join(dataset.model_path, "point_cloud", "iteration_30000", "point_cloud.ply"))
    gaussians.save_fused_ply(args.output_ply)
    
