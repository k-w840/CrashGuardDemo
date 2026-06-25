#import "LogReportViewController.h"
#import "ZWMobileGuard.h"
#import "CrashDemoUtil.h"
#import <Masonry/Masonry.h>

#define DrawingStatusCellIdentify  @"DrawingStatusCell"
#define LogReportControlCellIdentify  @"LogReportControlCell"
#define LogPreviewCellCellIdentify  @"LogPreviewCell"

@interface DrawingStatusCell : UITableViewCell
@property (nonatomic, strong) UILabel *statusLabel;
@end

@implementation DrawingStatusCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        [self.contentView addSubview:self.statusLabel];
        [self.statusLabel mas_makeConstraints:^(MASConstraintMaker *make) {
            make.left.top.equalTo(self.contentView).offset(20.0);
        }];
    }
    return self;
}

- (UILabel *)statusLabel {
    if (!_statusLabel) {
        _statusLabel = [[UILabel alloc] init];
        _statusLabel.textColor = [UIColor blackColor];
        _statusLabel.font = [UIFont systemFontOfSize:13 weight:UIFontWeightMedium];
        _statusLabel.numberOfLines = 0;
    }
    return _statusLabel;
}

@end

@class LogReportControlCell;
@protocol LogReportControlCellDelegate <NSObject>
- (void)logReportControlCell:(LogReportControlCell *)cell didToggleSwitch:(BOOL)isOn;
- (void)logReportControlCellDidTapSync:(LogReportControlCell *)cell;
- (void)logReportControlCellDidTapClear:(LogReportControlCell *)cell;
@end

@interface LogReportControlCell : UITableViewCell
@property (nonatomic, weak) id<LogReportControlCellDelegate> delegate;
@property (nonatomic, strong) UILabel *logCountLabel;
@property (nonatomic, strong) UISwitch *drawingUploadSwitch;
@end

@implementation LogReportControlCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        [self.contentView addSubview:self.logCountLabel];
        [self.logCountLabel mas_makeConstraints:^(MASConstraintMaker *make) {
            make.top.equalTo(self.contentView).offset(16.0);
            make.left.equalTo(self.contentView).offset(20.0);
        }];
        
        UILabel *switchLabel = [[UILabel alloc] init];
        switchLabel.text = @"GDPR/IP 授权: 允许上传图纸本体";
        switchLabel.textColor = [UIColor blackColor];
        switchLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightBold];
        [self.contentView addSubview:switchLabel];
        [switchLabel mas_makeConstraints:^(MASConstraintMaker *make) {
            make.top.equalTo(self.logCountLabel.mas_bottom).offset(16.0);
            make.left.equalTo(self.logCountLabel);
        }];
        
        [self.contentView addSubview:self.drawingUploadSwitch];
        [self.drawingUploadSwitch mas_makeConstraints:^(MASConstraintMaker *make) {
            make.centerY.equalTo(switchLabel);
            make.left.equalTo(switchLabel.mas_right).offset(12.0);
        }];
        
        UIButton *syncButton = [self operationBtnWithTitle:@"同步崩溃报告" selector:@selector(syncTapped:)];
        [self.contentView addSubview:syncButton];
        [syncButton mas_makeConstraints:^(MASConstraintMaker *make) {
            make.top.equalTo(switchLabel.mas_bottom).offset(16.0);
            make.left.equalTo(self.logCountLabel);
            make.size.mas_equalTo(CGSizeMake(80, 40));
        }];
        
        UIButton *clearButton = [self operationBtnWithTitle:@"清空本地日志" selector:@selector(clearTapped:)];
        [self.contentView addSubview:clearButton];
        [clearButton mas_makeConstraints:^(MASConstraintMaker *make) {
            make.top.equalTo(syncButton);
            make.left.equalTo(syncButton.mas_right).offset(12.0);
            make.size.mas_equalTo(CGSizeMake(80, 40));
        }];
    }
    return self;
}

- (void)switchToggled:(UISwitch *)sender {
    if ([self.delegate respondsToSelector:@selector(logReportControlCell:didToggleSwitch:)]) {
        [self.delegate logReportControlCell:self didToggleSwitch:sender.isOn];
    }
}

- (void)syncTapped:(UIButton *)sender {
    if ([self.delegate respondsToSelector:@selector(logReportControlCellDidTapSync:)]) {
        [self.delegate logReportControlCellDidTapSync:self];
    }
}

- (void)clearTapped:(UIButton *)sender {
    if ([self.delegate respondsToSelector:@selector(logReportControlCellDidTapClear:)]) {
        [self.delegate logReportControlCellDidTapClear:self];
    }
}

- (UIButton *)operationBtnWithTitle:(NSString *)title selector:(SEL)selector {
    UIButton *operationButton = [UIButton buttonWithType:UIButtonTypeCustom];
    [operationButton setTitle:title forState:UIControlStateNormal];
    [operationButton setTitleColor:[UIColor blackColor] forState:UIControlStateNormal];
    operationButton.titleLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightBold];
    [operationButton addTarget:self action:selector forControlEvents:UIControlEventTouchUpInside];
    return operationButton;
}

- (UILabel *)logCountLabel {
    if (!_logCountLabel) {
        _logCountLabel = [[UILabel alloc] init];
        _logCountLabel.textColor = [UIColor blackColor];
        _logCountLabel.font = [UIFont systemFontOfSize:12 weight:UIFontWeightBold];
        _logCountLabel.text = @"本地崩溃日志: 0 条";
    }
    return _logCountLabel;
}

- (UISwitch *)drawingUploadSwitch {
    if (!_drawingUploadSwitch) {
        _drawingUploadSwitch = [[UISwitch alloc] init];
        _drawingUploadSwitch.on = NO;
        [_drawingUploadSwitch addTarget:self action:@selector(switchToggled:) forControlEvents:UIControlEventValueChanged];
    }
    return _drawingUploadSwitch;
}

@end

@interface LogPreviewCell : UITableViewCell
@property (nonatomic, strong) UITextView *logTextView;
@end

@implementation LogPreviewCell

- (instancetype)initWithStyle:(UITableViewCellStyle)style reuseIdentifier:(NSString *)reuseIdentifier {
    self = [super initWithStyle:style reuseIdentifier:reuseIdentifier];
    if (self) {
        [self.contentView addSubview:self.logTextView];
        [self.logTextView mas_makeConstraints:^(MASConstraintMaker *make) {
            make.edges.equalTo(self.contentView).insets(UIEdgeInsetsMake(12.0, 12.0, 12.0, 12.0));
            make.height.mas_equalTo(120.0);
        }];
    }
    return self;
}

- (UITextView *)logTextView {
    if (!_logTextView) {
        _logTextView = [[UITextView alloc] initWithFrame:CGRectZero];
        _logTextView.backgroundColor = [UIColor blackColor];
        _logTextView.textColor = [UIColor greenColor];
        _logTextView.font = [UIFont systemFontOfSize:10];
        _logTextView.editable = NO;
        _logTextView.layer.cornerRadius = 6.0;
        _logTextView.text = @"暂未选择任何崩溃日志进行预览...";
        _logTextView.textContainerInset = UIEdgeInsetsMake(8.0, 6.0, 8.0, 6.0);
    }
    return _logTextView;
}

@end



@interface LogReportViewController () <UITableViewDelegate, UITableViewDataSource, LogReportControlCellDelegate>

@property (nonatomic, strong) UITableView *tableView;
@property (nonatomic, strong) NSArray<NSString *> *crashLogPaths;
@property (nonatomic, assign) BOOL drawingUploadAuth;
@property (nonatomic, strong) NSString *currentPreviewText;

@end

@implementation LogReportViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = @"日志上报";
    
    self.drawingUploadAuth = NO;
    self.currentPreviewText = @"暂未选择任何崩溃日志进行预览...";
    
    [self.view addSubview:self.tableView];
    [self.tableView mas_makeConstraints:^(MASConstraintMaker *make) {
        make.edges.equalTo(self.view);
    }];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [self reloadCrashLogsList];
}

#pragma mark - Actions

- (void)reloadCrashLogsList {
    self.crashLogPaths = [[ZWMobileGuard sharedInstance] getCrashLogPaths];
    [self.tableView reloadData];
}

- (void)showLogMessage:(NSString *)msg {
    self.currentPreviewText = msg;
    NSIndexPath *indexPath = [NSIndexPath indexPathForRow:1 inSection:1];
    LogPreviewCell *cell = [self.tableView cellForRowAtIndexPath:indexPath];
    if (cell) {
        cell.logTextView.text = msg;
    }
}

#pragma mark - LogReportControlCellDelegate

- (void)logReportControlCell:(LogReportControlCell *)cell didToggleSwitch:(BOOL)isOn {
    self.drawingUploadAuth = isOn;
}

- (void)logReportControlCellDidTapSync:(LogReportControlCell *)cell {
    if (self.crashLogPaths.count == 0) {
        [self showLogMessage:@"本地没有未上传的崩溃日志。"];
        return;
    }

    NSString *logPath = self.crashLogPaths[0];
    BOOL uploadAuth = self.drawingUploadAuth;

    [self showLogMessage:@"正在同步崩溃数据到分析看板..."];

    [[ZWMobileGuard sharedInstance] uploadCrashLogAtPath:logPath uploadDrawing:uploadAuth completion:^(BOOL success, NSString *message) {
        if (success) {
            [self showLogMessage:message];
            [[ZWMobileGuard sharedInstance] deleteCrashLogAtPath:logPath];
            [self reloadCrashLogsList];
        } else {
            [self showLogMessage:[NSString stringWithFormat:@"上报失败: %@", message]];
        }
    }];
}

- (void)logReportControlCellDidTapClear:(LogReportControlCell *)cell {
    for (NSString *path in self.crashLogPaths) {
        [[ZWMobileGuard sharedInstance] deleteCrashLogAtPath:path];
    }
    [self reloadCrashLogsList];
    [self showLogMessage:@"本地历史崩溃日志已全部清空。"];
}

#pragma mark - UITableViewDelegate & UITableViewDataSource

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 3;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    if (section == 0) {
        return 1;
    }
    if (section == 1) {
        return 2;
    }
    return self.crashLogPaths.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == 0) {
        DrawingStatusCell *cell = [tableView dequeueReusableCellWithIdentifier:DrawingStatusCellIdentify forIndexPath:indexPath];
        cell.statusLabel.text = g_currentDrawingStatus;
        return cell;
    }
    if (indexPath.section == 1) {
        if (indexPath.row == 0) {
            LogReportControlCell *cell = [tableView dequeueReusableCellWithIdentifier:LogReportControlCellIdentify forIndexPath:indexPath];
            cell.delegate = self;
            cell.drawingUploadSwitch.on = self.drawingUploadAuth;
            cell.logCountLabel.text = [NSString stringWithFormat:@"本地崩溃日志: %lu 条", (unsigned long)self.crashLogPaths.count];
            return cell;
        }
        LogPreviewCell *cell = [tableView dequeueReusableCellWithIdentifier:LogPreviewCellCellIdentify forIndexPath:indexPath];
        cell.logTextView.text = self.currentPreviewText;
        return cell;
    }
    static NSString *cellID = @"LogFileCell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:cellID];
    if (!cell) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle reuseIdentifier:cellID];
        
        UIView *selectedBg = [[UIView alloc] init];
        selectedBg.backgroundColor = [UIColor blueColor];
        cell.selectedBackgroundView = selectedBg;
        
        cell.textLabel.textColor = [UIColor blackColor];
        cell.textLabel.font = [UIFont systemFontOfSize:11];
        cell.detailTextLabel.textColor = [UIColor lightGrayColor];
        cell.detailTextLabel.font = [UIFont systemFontOfSize:9];
    }
    
    NSString *path = [self.crashLogPaths objectAtIndex:indexPath.row];
    cell.textLabel.text = path.lastPathComponent;
    cell.detailTextLabel.text = path;
    return cell;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
    static NSString *headerID = @"LogReportHeaderView";
    UITableViewHeaderFooterView *header = [tableView dequeueReusableHeaderFooterViewWithIdentifier:headerID];
    if (!header) {
        header = [[UITableViewHeaderFooterView alloc] initWithReuseIdentifier:headerID];
        UIView *backgroundView = [[UIView alloc] init];
        backgroundView.backgroundColor = [UIColor lightGrayColor];
        header.backgroundView = backgroundView;
    }
    
    NSString *title = @"";
    if (section == 0) {
        title = @"活跃图纸状态";
    } else if (section == 1) {
        title = @"历史日志与合规同步上报";
    } else if (section == 2) {
        title = @"本地崩溃日志列表";
    }
    
    if (@available(iOS 14.0, *)) {
        UIListContentConfiguration *config = [header defaultContentConfiguration];
        config.text = title;
        config.textProperties.color = [UIColor blackColor];
        config.textProperties.font = [UIFont boldSystemFontOfSize:14.0];
        header.contentConfiguration = config;
    } else {
        header.textLabel.text = title;
        header.textLabel.textColor = [UIColor blackColor];
        header.textLabel.font = [UIFont boldSystemFontOfSize:14.0];
    }
    return header;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == 2) {
        NSString *path = self.crashLogPaths[indexPath.row];
        NSString *content = [[ZWMobileGuard sharedInstance] readCrashLogContentAtPath:path];
        [self showLogMessage:content];
    } else {
        [tableView deselectRowAtIndexPath:indexPath animated:NO];
    }
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
    return 36.0;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    if (indexPath.section == 0) {
        return 88.0;
    }
    if (indexPath.section == 1) {
        if (indexPath.row == 0) {
            return 140.0;
        }
        return 144.0;
    }
    return 52.0;
}

#pragma mark - Lazy

- (UITableView *)tableView {
    if (!_tableView) {
        _tableView = [[UITableView alloc] initWithFrame:CGRectZero style:UITableViewStylePlain];
        _tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
        _tableView.delegate = self;
        _tableView.dataSource = self;
        _tableView.estimatedRowHeight = 0;
        _tableView.estimatedSectionHeaderHeight = 0;
        _tableView.estimatedSectionFooterHeight = 0;
        _tableView.sectionFooterHeight = 0.0;
        if (@available(iOS 15.0, *)) {
            _tableView.sectionHeaderTopPadding = 0.0;
        }
        [_tableView registerClass:[DrawingStatusCell class] forCellReuseIdentifier:DrawingStatusCellIdentify];
        [_tableView registerClass:[LogReportControlCell class] forCellReuseIdentifier:LogReportControlCellIdentify];
        [_tableView registerClass:[LogPreviewCell class] forCellReuseIdentifier:LogPreviewCellCellIdentify];
    }
    return _tableView;
}

@end
