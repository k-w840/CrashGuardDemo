#import "StaticTabBarController.h"
#import <Masonry/Masonry.h>

@interface StaticTabBarButton : UIControl

@property (nonatomic, strong) UIImageView *iconView;
@property (nonatomic, strong) UILabel *titleLabel;

@end

@implementation StaticTabBarButton

- (instancetype)init {
    self = [super init];
    if (self) {
        self.backgroundColor = [UIColor clearColor];
        [self addSubview:self.iconView];
        [self addSubview:self.titleLabel];

        [self.iconView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.centerX.equalTo(self);
            make.top.equalTo(self.mas_top).offset(4.0);
            make.size.mas_equalTo(CGSizeMake(22.0, 22.0));
        }];

        [self.titleLabel mas_makeConstraints:^(MASConstraintMaker *make) {
            make.leading.trailing.equalTo(self);
            make.top.equalTo(self.iconView.mas_bottom).offset(2.0);
            make.bottom.lessThanOrEqualTo(self.mas_bottom).offset(-2.0);
        }];
    }
    return self;
}

- (UILabel *)titleLabel {
    if (!_titleLabel) {
        _titleLabel = [[UILabel alloc] init];
        _titleLabel.textAlignment = NSTextAlignmentCenter;
        _titleLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightMedium];
    }
    return _titleLabel;
}

- (UIImageView *)iconView {
    if (!_iconView) {
        _iconView = [[UIImageView alloc] init];
        _iconView.contentMode = UIViewContentModeScaleAspectFit;
    }
    return _iconView;
}

@end

@interface StaticTabBarController ()

@property (nonatomic, strong) UIView *customTabBarView;
@property (nonatomic, strong) NSMutableArray<StaticTabBarButton *> *tabButtons;

@end

@implementation StaticTabBarController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.view.backgroundColor = [UIColor blackColor];
    self.tabBar.hidden = YES;
    [self setupCustomTabBar];
}

- (void)setViewControllers:(NSArray<__kindof UIViewController *> *)viewControllers {
    [super setViewControllers:viewControllers];
    [self rebuildTabButtons];
}

- (void)setSelectedIndex:(NSUInteger)selectedIndex {
    [super setSelectedIndex:selectedIndex];
    [self updateTabSelection];
}

#pragma mark - private
// 创建自定义tab视图
- (void)setupCustomTabBar {
    if (_customTabBarView.superview) {
        return;
    }

    [self.view addSubview:self.customTabBarView];
    [self.customTabBarView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.leading.trailing.bottom.equalTo(self.view);
        make.top.equalTo(self.view.mas_safeAreaLayoutGuideBottom).offset(-49.0);
    }];
}

// 构建tab按钮
- (void)rebuildTabButtons {
    StaticTabBarButton *previousButton = nil;
    for (NSUInteger index = 0; index < self.viewControllers.count; index++) {
        StaticTabBarButton *button = [[StaticTabBarButton alloc] init];
        button.tag = (NSInteger)index;
        [button addTarget:self action:@selector(handleTabButtonTap:) forControlEvents:UIControlEventTouchUpInside];
        [self.customTabBarView addSubview:button];

        [button mas_makeConstraints:^(MASConstraintMaker *make) {
            make.top.equalTo(self.customTabBarView);
            make.bottom.equalTo(self.customTabBarView.mas_safeAreaLayoutGuideBottom);
            if (previousButton != nil) {
                make.leading.equalTo(previousButton.mas_trailing);
                make.width.equalTo(previousButton);
            } else {
                make.leading.equalTo(self.customTabBarView);
            }
        }];

        previousButton = button;
        [self.tabButtons addObject:button];
        [self configStaticBtnStyle:button withTabBarItem:self.viewControllers[index].tabBarItem isSelected:index == self.selectedIndex];
    }

    if (previousButton) {
        [previousButton mas_makeConstraints:^(MASConstraintMaker *make) {
            make.trailing.equalTo(self.customTabBarView);
        }];
    }

}

// 更新tab按钮选中状态
- (void)updateTabSelection {
    for (NSUInteger index = 0; index < self.tabButtons.count; index++) {
        StaticTabBarButton *button = self.tabButtons[index];
        [self configStaticBtnStyle:button withTabBarItem:self.viewControllers[index].tabBarItem isSelected:index == self.selectedIndex];
    }
}

// 配置静态按钮样式
- (void)configStaticBtnStyle:(StaticTabBarButton *)button withTabBarItem:(UITabBarItem *)item isSelected:(BOOL)isSelected {
    button.titleLabel.text = item.title;
    if (isSelected) {
        UIColor *selectedColor = [UIColor colorWithRed:0.10 green:0.49 blue:0.97 alpha:1.0];
        button.iconView.image = item.selectedImage;
        button.iconView.tintColor = selectedColor;
        button.titleLabel.textColor = selectedColor;
    } else {
        UIColor *normalColor = [UIColor colorWithRed:0.38 green:0.40 blue:0.43 alpha:1.0];
        button.iconView.image = item.image;
        button.iconView.tintColor = normalColor;
        button.titleLabel.textColor = normalColor;
    }
}

#pragma mark - clickedAction
- (void)handleTabButtonTap:(StaticTabBarButton *)sender {
    NSUInteger targetIndex = (NSUInteger)sender.tag;
    if (targetIndex >= self.viewControllers.count || targetIndex == self.selectedIndex) {
        return;
    }

    [UIView performWithoutAnimation:^{
        self.selectedIndex = targetIndex;
        [self.view layoutIfNeeded];
    }];
}

#pragma mark - lazy
- (UIView *)customTabBarView {
    if (!_customTabBarView) {
        _customTabBarView = [[UIView alloc] initWithFrame:CGRectZero];
        _customTabBarView.backgroundColor = [UIColor colorWithRed:45.0/255.0 green:45.0/255.0 blue:45.0/255.0 alpha:1.0];
    }
    return _customTabBarView;
}

- (NSMutableArray<StaticTabBarButton *> *)tabButtons {
    if (!_tabButtons) {
        _tabButtons = [[NSMutableArray alloc] init];
    }
    return _tabButtons;
}

@end
