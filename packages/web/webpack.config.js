// process.traceDeprecation = true;

const path = require('node:path');
const process = require('node:process');
const HtmlWebpackPlugin = require('html-webpack-plugin');
const MiniCssExtractPlugin = require('mini-css-extract-plugin');
const UglifyJsPlugin = require('uglifyjs-webpack-plugin');
const OptimizeCSSAssetsPlugin = require('optimize-css-assets-webpack-plugin');
const OfflinePlugin = require('offline-plugin');
const HtmlWebpackInlineSourcePlugin = require('html-webpack-inline-source-plugin');
const FaviconsWebpackPlugin = require('favicons-webpack-plugin');
const WebpackPwaManifest = require('webpack-pwa-manifest');
const GitRevisionPlugin = require('git-revision-webpack-plugin');
const RemoveAssetsPlugin = require('remove-assets-webpack-plugin');
const BundleAnalyzerPlugin = require('webpack-bundle-analyzer').BundleAnalyzerPlugin;

const title = 'Robot Odyssey Rewired';
const icon = require.resolve('@robot-odyssey-rewired/engine/dist/images/icon.png');

module.exports = {
    mode: process.env.DEBUG ? 'development' : 'production',
    entry: './src/main.ts',
    output: {
        filename: '[name].[hash].js',
        path: path.resolve(__dirname, 'dist')
    },
    resolve: {
        extensions: ['.ts', '.js']
    },
    stats: {
        colors: true,
        children: true,
        chunks: true,
        chunkModules: true,
        modules: true,
    },
    node: {
        fs: 'empty'
    },
    performance: {
        // The WASM is getting included in the entry point size, even though
        // webpack here is only processing the URL of the resource and we're still
        // downloading it asynchronously. Oh well.
        hints: false,
    },
    plugins: [

        // CSS and HTML inlined

        new MiniCssExtractPlugin({
            filename: '[name].[hash].css'
        }),
        new HtmlWebpackPlugin({
            template: './src/main.html',
            inlineSource: '.css$',
            title,
            version: (new GitRevisionPlugin({
                versionCommand: 'describe --always --tags --dirty',
            })).version(),
        }),
        new HtmlWebpackInlineSourcePlugin(),
        new RemoveAssetsPlugin(/\.css$/),

        // Generate content for the progressive web app

        new FaviconsWebpackPlugin({
            logo: icon,
            prefix: 'icon.[hash].',
            persistentCache: false,
            title,
            background: '#000',
            icons: {
                favicons: true,         // useful
                appleIcon: true,        // useful
                appleStartup: false,    // broken; some are rotated?
                    android: false,         // No PWA manifest; that's below
                firefox: false,         // No PWA manifest; that's below
            }
        }),
        new WebpackPwaManifest({
            name: title,
            background_color: '#000000',
            icons: [
                {
                    src: icon,
                    sizes: [32, 128, 256, 512]
                }
            ],
        }),

        // Offline plugin must be AFTER all resources we want to cache.
        new OfflinePlugin(),

        // Analyze resource sizes.
        // By putting this after OfflinePlugin it stays out of our sw.js bundle,
        // and put the output in our build dir rather than 'dist' so it isn't uploaded.
        new BundleAnalyzerPlugin({
            analyzerMode: 'static',
            defaultSizes: 'gzip',
            openAnalyzer: false,
            reportFilename: '../build/bundle-analyzer-report.html',
        }),
    ],
    module: {
        rules: [
            {
                test: /\.css$/,
                use: [ MiniCssExtractPlugin.loader, 'css-loader' ]
            },
            {
                test: /\.(png|gif|woff|woff2)$/,
                loader: 'url-loader',
                options: {
                    limit: 8192,
                    fallback: 'file-loader',
                    name: '[name]-[hash].[ext]',
                }
            },
            // Note, currently avoiding using a .wasm extension for the engine binary
            // as this version of Webpack has some early wasm support that doesn't work and
            // I'm not sure how to disable it. Clearing defaultRules isn't sufficient.
            // The deployed file will still be called engine-[hash].wasm
            {
                test: /\/engine\/dist\/wasm$/,
                loader: 'file-loader',
                options: {
                    name: 'engine-[hash].wasm',
                }
            },
            {
                test: /\.ts$/,
                use: 'ts-loader'
            }
        ]
    },
    optimization: {
        minimizer: [
            new UglifyJsPlugin({}),
            new OptimizeCSSAssetsPlugin({
                cssProcessor: require('cssnano'),
                cssProcessorOptions: {
                    zindex: false,
                },
                canPrint: true,
            })
        ],
        splitChunks: {
            cacheGroups: {
                vendors: false,
                default: false,
            }
        }
    },
};

