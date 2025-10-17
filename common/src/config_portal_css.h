// Config Portal CSS - Mobile-optimized styles for configuration web interface
// This file contains the CSS for the configuration portal that runs in AP mode
// and config mode. The CSS is embedded in the firmware to avoid filesystem dependencies.

#ifndef CONFIG_PORTAL_CSS_H
#define CONFIG_PORTAL_CSS_H

const char* CONFIG_PORTAL_CSS = R"(
<style>
    /* Reset and base styles */
    * {
        margin: 0;
        padding: 0;
        box-sizing: border-box;
    }
    
    body {
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        min-height: 100vh;
        padding: 20px;
    }
    
    /* Container */
    .container {
        background: white;
        border-radius: 12px;
        box-shadow: 0 10px 40px rgba(0,0,0,0.2);
        padding: 40px;
        max-width: 500px;
        width: 100%;
        margin: 0 auto;
    }
    
    /* Typography */
    h1 {
        color: #333;
        margin-bottom: 10px;
        font-size: 28px;
    }
    
    .subtitle {
        color: #666;
        margin-bottom: 30px;
        font-size: 14px;
    }
    
    /* Form elements */
    .form-group {
        margin-bottom: 20px;
    }
    
    label {
        display: block;
        margin-bottom: 8px;
        color: #333;
        font-weight: 500;
        font-size: 14px;
    }
    
    input[type="text"],
    input[type="password"],
    input[type="number"],
    input[type="url"] {
        width: 100%;
        padding: 12px;
        border: 2px solid #e0e0e0;
        border-radius: 6px;
        font-size: 16px;
        transition: border-color 0.3s;
        -webkit-appearance: none;
        touch-action: manipulation;
    }
    
    input:focus {
        outline: none;
        border-color: #667eea;
    }
    
    /* Checkbox styling */
    input[type="checkbox"] {
        width: 20px;
        height: 20px;
        cursor: pointer;
        touch-action: manipulation;
    }
    
    .help-text {
        font-size: 12px;
        color: #666;
        margin-top: 5px;
    }
    
    /* Buttons */
    button {
        width: 100%;
        padding: 14px;
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        border: none;
        border-radius: 6px;
        font-size: 16px;
        font-weight: 600;
        cursor: pointer;
        transition: transform 0.2s;
        touch-action: manipulation;
        -webkit-tap-highlight-color: rgba(0,0,0,0.1);
    }
    
    button:active {
        transform: translateY(0);
    }
    
    .btn-secondary {
        background: linear-gradient(135deg, #10b981 0%, #059669 100%);
        padding: 12px 24px;
        font-size: 15px;
        width: 100%;
    }
    
    /* Desktop hover effect */
    @media (hover: hover) {
        button:hover {
            transform: translateY(-2px);
        }
    }
    
    /* Status messages */
    .success {
        background: #10b981;
        color: white;
        padding: 20px;
        border-radius: 8px;
        text-align: center;
    }
    
    .error {
        background: #ef4444;
        color: white;
        padding: 20px;
        border-radius: 8px;
        text-align: center;
    }
    
    /* Device info */
    .device-info {
        background: #f3f4f6;
        padding: 15px;
        border-radius: 6px;
        margin-bottom: 20px;
        font-size: 14px;
        word-break: break-word;
    }
    
    .device-info strong {
        color: #667eea;
    }
    
    /* Factory reset section */
    .factory-reset-section {
        margin-top: 30px;
        padding-top: 30px;
        border-top: 2px solid #e0e0e0;
    }
    
    .danger-zone {
        background: #fef2f2;
        border: 2px solid #fee2e2;
        border-radius: 8px;
        padding: 20px;
    }
    
    .danger-zone h2 {
        color: #dc2626;
        font-size: 18px;
        margin-bottom: 10px;
    }
    
    .danger-zone p {
        color: #666;
        font-size: 14px;
        margin-bottom: 15px;
    }
    
    .btn-danger {
        background: #dc2626;
        color: white;
        padding: 12px 24px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        transition: background 0.3s;
        touch-action: manipulation;
    }
    
    @media (hover: hover) {
        .btn-danger:hover {
            background: #b91c1c;
        }
    }
    
    /* Modal */
    .modal {
        display: none;
        position: fixed;
        z-index: 1000;
        left: 0;
        top: 0;
        width: 100%;
        height: 100%;
        background-color: rgba(0,0,0,0.5);
        overflow-y: auto;
    }
    
    .modal-content {
        background-color: white;
        position: absolute;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        padding: 30px;
        border-radius: 12px;
        max-width: 400px;
        width: 90%;
        box-shadow: 0 10px 40px rgba(0,0,0,0.3);
        -webkit-overflow-scrolling: touch;
    }
    
    .modal-buttons {
        display: flex;
        gap: 10px;
        margin-top: 20px;
    }
    
    .btn-cancel {
        flex: 1;
        background: #6b7280;
        color: white;
        padding: 12px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        touch-action: manipulation;
    }
    
    .btn-confirm {
        flex: 1;
        background: #dc2626;
        color: white;
        padding: 12px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        touch-action: manipulation;
    }
    
    /* OTA Update Styles */
    .warning-banner {
        background: #fef3c7;
        border: 2px solid #f59e0b;
        border-radius: 8px;
        padding: 15px;
        margin: 20px 0;
        color: #92400e;
        font-size: 14px;
        line-height: 1.6;
    }
    
    .warning-banner strong {
        color: #78350f;
    }
    
    .progress-container {
        width: 100%;
        background-color: #e5e7eb;
        border-radius: 8px;
        overflow: hidden;
        height: 40px;
        margin-top: 15px;
    }
    
    .progress-bar {
        height: 100%;
        background: linear-gradient(90deg, #667eea 0%, #764ba2 100%);
        color: white;
        display: flex;
        align-items: center;
        justify-content: center;
        font-weight: 600;
        font-size: 14px;
        transition: width 0.3s ease;
        width: 0%;
    }
    
    .btn-primary {
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        padding: 12px 24px;
        border: none;
        border-radius: 6px;
        font-size: 14px;
        font-weight: 600;
        cursor: pointer;
        touch-action: manipulation;
        transition: opacity 0.2s;
    }
    
    .btn-primary:hover {
        opacity: 0.9;
    }
    
    .btn-primary:disabled {
        opacity: 0.5;
        cursor: not-allowed;
    }
    
    input[type="file"] {
        padding: 10px;
        border: 2px dashed #cbd5e1;
        border-radius: 6px;
        width: 100%;
        cursor: pointer;
        background: #f9fafb;
    }
    
    input[type="file"]:hover {
        border-color: #667eea;
        background: #f3f4f6;
    }
    
    /* Mobile responsive adjustments */
    @media (max-width: 600px) {
        body {
            padding: 12px;
        }
        
        .container {
            padding: 20px;
        }
        
        h1 {
            font-size: 24px;
        }
        
        .modal-content {
            width: 95%;
            padding: 20px;
        }
    }
</style>
)";

#endif // CONFIG_PORTAL_CSS_H
